#include "QwPitaFeedback.h"
#include "VQwDataHandler.h"
#include "QwParameterFile.h"
#include "QwFactory.h"
#include "QwUnits.h"

#include "QwMollerADC_Channel.h"

#include <stdexcept>


// Register this handler with the factory
// RegisterHandlerFactory(QwPitaFeedback);
template<>
const VQwDataHandlerFactory* MQwCloneable<VQwDataHandler,QwPitaFeedback>::fFactory = new QwFactory<VQwDataHandler,QwPitaFeedback>("QwPitaFeedback");

QwPitaFeedback::QwPitaFeedback(const TString& name)
: VQwDataHandler(name)
, CorrectionRoutine()
, options()
, slope{nullptr}
{

	ParseSeparator = "_";
	fKeepRunningSum = kTRUE; // We require the Charge Asymmetry
							 // for PITA Feedback
}

QwPitaFeedback::QwPitaFeedback(const QwPitaFeedback &source)
: VQwDataHandler(source)
, CorrectionRoutine(source.CorrectionRoutine)
, options(source.options)
, slope{nullptr}
{
	if(source.slope)
		slope = std::make_unique<pita_slope>(*source.slope);
}

QwPitaFeedback::~QwPitaFeedback() { }

Int_t QwPitaFeedback::LoadChannelMap(const std::string& mapfile)
{
	QwMessage << "Loading Channel Map: " << mapfile << QwLog::endl;
	if(options.ihwp_ioc_channel.size()) {
		slope = std::make_unique<pita_slope>(options.ihwp_ioc_channel, options.slope_in, options.slope_out, options.revert_ihwp_state);
	} else {
		slope = std::make_unique<pita_slope>(options.slope_in, options.slope_out, options.revert_ihwp_state);
	}

	// Open the Parameter File
	QwParameterFile map(mapfile);

	// Read the sections of dependent variables
	std::pair<EQwHandleType, std::string> type_name;

	// Loop over mapfile and look for keys
	// Format: [label] [type]_[name]
	//    ie :     dv    asym_tq05r2
	while(map.ReadNextLine()) {
		QwMessage << "QwPitaFeedback is reading the next line\n";
		// Remove comments, spaces, and empty lines.
		map.TrimComment();
		map.TrimWhitespace();
		if(map.LineIsEmpty()) continue;

		// Get first token: dv, iv, ep (dependent, idependent, epics)
		// Get second token [asym, diff, mps]
		std::string primary_token = map.GetNextToken(" ");
		std::string current_token = map.GetNextToken(" ");

		// Looks for type_<name>
		// mps/asym/yield/diff_<name>
		type_name = ParseHandledVariable(current_token);

		if(primary_token == "dv") {
			QwMessage << "Found Primary Token dv: " << type_name.first << " " << type_name.second << QwLog::endl;
			// The Charge Asymmetry depends on these Parameters
			fDependentType.push_back( type_name.first  );
			fDependentName.push_back( type_name.second );
			fDependentFull.push_back(  current_token   );
		} else if (primary_token == "ep") {
			fEPICSType.push_back(  type_name.first  );
			fEPICSName.push_back(  type_name.second );
			fEPICSFull.push_back(  current_token    );
		} else {
			QwError << "LoadChannelMan in QwCorrelator read invalid primary token: " << primary_token << QwLog::endl;
		}
	}

	return 0;
}

void QwPitaFeedback::ParseConfigFile(QwParameterFile& file)
{	
	// Construct a list of parameters we would want to load from a config
	// here. For now, we are going to hardcoded everything.
	// Once we determine what we need to implement the correction routines,
	// then we can go back an modify the configuration impl.
	/* List of things to (?) configure
		[x] ihwp-ioc
		[x] pita slopes (ihwp =  IN)
		[x] pita slopes (ihwp = OUT)
		[x] ihwp reversal(?)
		[x] IOC setpoints to apply corrections
		[x] Patterns Required before feedback is performed
		    Note: The feedback will actuate on N-1 because of the order
			      in which we call ProcessData & AccumulateRunningSum
				  in QwDataHandler
		[ ] CorrectionRoutine

	*/
	VQwDataHandler::ParseConfigFile(file);
	
	unsigned corr_routine_conf = static_cast<unsigned>(CORRECTION_ROUTINE::UNKNOWN);
	file.PopValue("routine",corr_routine_conf);
	switch(static_cast<CORRECTION_ROUTINE>(corr_routine_conf)) {
	case CORRECTION_ROUTINE::ASYM:
		CorrectionRoutine = &QwPitaFeedback::AsymCorrection;
	break;
	case CORRECTION_ROUTINE::SYM:
		CorrectionRoutine = &QwPitaFeedback::SymCorrection;
	break;
	case CORRECTION_ROUTINE::XOR:
		CorrectionRoutine = &QwPitaFeedback::XORCorrection;
	break;
	default:
		throw std::runtime_error("Correction Routine not defined!\n\
		                          Add 'routine = {asym, sym, xor, etc.}' to the .conf file.");
	break;
	}

	file.PopValue("num-patterns"     , options.num_patterns    );
	file.PopValue("ihwp-ioc"         , options.ihwp_ioc_channel);
	file.PopValue("revert-ihwp-state", options.revert_ihwp_state);
  	if(!file.PopValue("slope-ihwp-in",  options.slope_in) || !file.PopValue("slope-ihwp-out", options.slope_out)) {
		throw std::runtime_error("PITA Slopes for IHWP-IN(OUT) not defined!\n\
		                          Add slope-ihwp-in(out) to the .conf file.");
	}
		
}


Int_t QwPitaFeedback::ConnectChannels(QwSubsystemArrayParity& detectors)
{
	QwWarning << "QwPitaFeedback::ConnectChannels is not implemented!" << QwLog::endl;
	return 0;
}

Int_t QwPitaFeedback::ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
{
	SetEventcutErrorFlagPointer(asym.GetEventcutErrorFlagPointer());

	// Fill vector of pointers to the relevant data elements
	// This is how we "share" access

	// Get dependent vars first
	for(size_t dv = 0; dv < fDependentName.size(); dv++) {
		if( fDependentType.at(dv) == kHandleTypeMps) {
			// Ignore MPS type when connecting to asymm or diff
			continue;
		}
		
		const VQwHardwareChannel *dv_ptr = this->RequestExternalPointer(fDependentFull.at(dv));
		if(dv_ptr == nullptr) {
			switch(fDependentType.at(dv)) {
			case kHandleTypeAsym:
				dv_ptr = asym.RequestExternalPointer(fDependentName.at(dv));
				break;
			case kHandleTypeDiff:
				dv_ptr = asym.RequestExternalPointer(fDependentName.at(dv));
				break;
			default:
				QwWarning << "(" << fName << ") QwPitaFeedback::ConnectChannels(QwSubsystemArrayParity& asym,QwSubsystemArrayParity& diff): Dependent variable, "
				          << fDependentName.at(dv)
						  << " was not found (fullname == "
						  << fDependentFull.at(dv) << ")" << QwLog::endl;
				break;
			}
		} // end if(dv_ptr == nullptr)

		// pair creation
		if(dv_ptr != nullptr) {
			fDependentVar.push_back(dv_ptr);
			fOutputVar.emplace_back(  dv_ptr->Clone(VQwDataElement::kDerived) );
		} else {
			QwWarning << "(" << fName << ") QwPitaFeedback::ConnectChannels(QwSubsystemArrayParity& asym,QwSubsystemArrayParity& diff): Dependent variable, "
				<< fDependentName.at(dv)
				<< " was not found (fullname == "
				<< fDependentFull.at(dv) << QwLog::endl;
		}
	}

	fDependentValues.resize(fDependentVar.size());
	fOutputValues.resize(fOutputVar.size());

	for(size_t ep = 0; ep < fEPICSName.size(); ep++) {
		bool valid = kTRUE;
		HELICITY hel = HELICITY::UNKNOWN;
		POLARITY pol = POLARITY::UNKNOWN;
		switch(fEPICSType.at(ep)) {
		case kHandleTypeHVPP:
			hel = HELICITY::POSITIVE;
			pol = POLARITY::POSITIVE;
		break;
		case kHandleTypeHVPN:
			hel = HELICITY::POSITIVE;
			pol = POLARITY::NEGATIVE;
		break;
		case kHandleTypeHVNN:
			hel = HELICITY::NEGATIVE;
			pol = POLARITY::NEGATIVE;
		break;
		case kHandleTypeHVNP:
			hel = HELICITY::NEGATIVE;
			pol = POLARITY::POSITIVE;
		break;
		default:
			valid = kFALSE;
			QwError << "UNKNOWN EPICS Type (" << fEPICSType.at(ep) << ")!" << QwLog::endl;
		}

		if(valid) {
			setpoints.emplace_back(fEPICSName.at(ep), pol, hel);
		}

	}

	return 0;
}

void QwPitaFeedback::ProcessData()
{
	for (size_t i = 0; i < fDependentVar.size(); ++i) {
    	fOutputVar[i]->AssignValueFrom(fDependentVar[i]);
	}
}

// This is a hack. We should derive a FeedbackHandler from QwDataHandler
// and abstract this logic there. The re-direction scheme is error prone
// and hard to follow -- mrc (09/19/25)
void QwPitaFeedback::AccumulateRunningSum(VQwDataHandler &value, Int_t count, Int_t ErrorMask)
{
	// this = fRunningsum
	VQwDataHandler::AccumulateRunningSum(value, count, ErrorMask);
	// well this is horrible re-indirection
	if(GetGoodEventCount(0) >= options.num_patterns) {
		QwMessage << "q_targ good events: " <<GetGoodEventCount(0)<< QwLog::endl;
		QwMessage << "NPatterns: " <<GetGoodEventCount(0)<< QwLog::endl;
		static_cast<QwPitaFeedback&>(value).CalculateRunningAverage();
		static_cast<QwPitaFeedback&>(value).CalcCorrection();
		ClearEventData();
	}
}
// I will need at least three CalcCorrection functions to support
// all of the feedbacks
// 	1) asymmetric (PITA)
//		-- Increasing the volage in one helicity state while
//		-- decreasing the voltage in the other helicity state
//	2) symmetric (IA)
//		-- Increase or Decrease all voltages equally
//  3) psuedo-XOR
//		-- If Hel == Pol, subtract
//		-- If Hel != Pol, add
// Create a std::function pointer for the different calc routines (could
// do a templated function ptr thing)
void QwPitaFeedback::CalcCorrection()
{
	QwMessage << "fRunningsum data:\n\t";
	QwMessage << "Mean: "  << fRunningsum->GetValue(0)      / Qw::ppm << "\n\t";
	QwMessage << "Error: " << fRunningsum->GetValueError(0) / Qw::ppm << "\n\t";
	QwMessage << "Width: " << fRunningsum->GetValueWidth(0) / Qw::ppm << QwLog::endl;
	
	double correction = fRunningsum->GetValue(0) / Qw::ppm / slope->GetSlope();
	QwMessage << "Correction value = " << correction << QwLog::endl;
	
	for(auto & v : setpoints) {
		CorrectionRoutine(this, v, correction);
	}
}


void QwPitaFeedback::AsymCorrection(RTP<double>& setpoint, const double correction)
{
	// 	1) asymmetric (PITA)
	//		-- Increasing the volage in one helicity state while
	//		-- decreasing the voltage in the other helicity state
	int sign = (setpoint.GetPolarization() == POLARITY::POSITIVE) ? -1 : +1;
	double curr_setpoint = setpoint.GetChannelValue();
	setpoint.SetChannelValue(curr_setpoint + sign * correction);
	QwMessage << "using AsymCorrection\n";
	QwMessage << "\tSign: " << sign << "\n";
	QwMessage << "\tCurr: " << curr_setpoint << "\n";
	QwMessage << "\tCorr: " << curr_setpoint + sign * correction << QwLog::endl;
}
void QwPitaFeedback::SymCorrection(RTP<double>& setpoint, const double correction)
{
	//	2) symmetric (IA)
	//		-- Increase or Decrease all voltages equally
	int sign = -1;
	double curr_setpoint = setpoint.GetChannelValue();
	setpoint.SetChannelValue(curr_setpoint + sign * correction);
	QwMessage << "using SymCorrection\n";
	QwMessage << "\tSign: " << sign << "\n";
	QwMessage << "\tCurr: " << curr_setpoint << "\n";
	QwMessage << "\tCorr: " << curr_setpoint + sign * correction << QwLog::endl;
}
void QwPitaFeedback::XORCorrection(RTP<double>& setpoint, const double correction)
{
	//  3) psuedo-XOR
	//		-- If Hel == Pol, subtract
	//		-- If Hel != Pol, add
	int sign = (setpoint.GetPolarization() == setpoint.GetHelicity()) ? -1 : +1;
	double curr_setpoint = setpoint.GetChannelValue();
	setpoint.SetChannelValue(curr_setpoint + sign * correction);
	QwMessage << "using XORCorrection\n";
	QwMessage << "\tSign: " << sign << "\n";
	QwMessage << "\tCurr: " << curr_setpoint << "\n";
	QwMessage << "\tCorr: " << curr_setpoint + sign * correction << QwLog::endl;
}
