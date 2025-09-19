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
	: VQwDataHandler(name),
	setpoint1("IGL0I00C1068_DAC05"),// For now, we are hardcoding these in
	setpoint2("IGL0I00C1068_DAC06"),// I would like to open them up to be
	setpoint3("IGL0I00C1068_DAC07"),// configurable from mapfiles
	setpoint4("IGL0I00C1068_DAC08"),
	setpoint5("IGL0I00C1068_DAC09"),
	setpoint6("IGL0I00C1068_DAC10"),
	setpoint7("IGL0I00C1068_DAC11"),
	setpoint8("IGL0I00C1068_DAC12"),
	options(),
	slope{nullptr}
{

	ParseSeparator = "_";
	fKeepRunningSum = kTRUE; // We require the Charge Asymmetry
							 // for PITA Feedback
}

QwPitaFeedback::QwPitaFeedback(const QwPitaFeedback &source)
	: VQwDataHandler(source),
	setpoint1(source.setpoint1),// For now, we are hardcoding these in
	setpoint2(source.setpoint2),// I would like to open them up to be
	setpoint3(source.setpoint3),// configurable from mapfiles
	setpoint4(source.setpoint4),
	setpoint5(source.setpoint5),
	setpoint6(source.setpoint6),
	setpoint7(source.setpoint7),
	setpoint8(source.setpoint8),
	options(source.options),
	slope{nullptr},
	NPatterns(source.NPatterns)
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
		} else {
			QwError << "LoadChannelMan in QwCorrelator read invalid primary token: " << primary_token << QwLog::endl;
		}
	}

	/* For Reassurance -- mrc */
	for(auto const & v : fDependentType)
		QwMessage << "fDependentType: " << v << QwLog::endl;
	for(auto const & v : fDependentName)
		QwMessage << "fDependentName: " << v << QwLog::endl;
	for(auto const & v : fDependentFull)
		QwMessage << "fDependentFull: " << v << QwLog::endl;

	return 0;
}

void QwPitaFeedback::ParseConfigFile(QwParameterFile& file)
{	
	// Construct a list of parameters we would want to load from a config
	// here. For now, we are going to hardcoded everything.
	// Once we determine what we need to implement the correction routines,
	// then we can go back an modify the configuration impl.
	/* List of things to (?) configure
		[ ] ihwp-ioc
		[ ] pita slopes (ihwp =  IN)
		[ ] pita slopes (ihwp = OUT)
		[ ] ihwp reversal(?)
		[ ] IOC setpoints to apply corrections
		[ ] Patterns Required before feedback is performed
		    Note: The feedback will actuate on N-1 because of the order
			      in which we call ProcessData & AccumulateRunningSum
				  in QwDataHandler


	*/
	VQwDataHandler::ParseConfigFile(file);

	QwWarning << "Getting PITA info from config file!" << QwLog::endl;

	file.PopValue("ihwp-ioc"         , options.ihwp_ioc_channel);
	file.PopValue("revert-ihwp-state", options.revert_ihwp_state);
  	if(!file.PopValue("slope-ihwp-in",  options.slope_in) || !file.PopValue("slope-ihwp-out", options.slope_out)) {
		throw std::runtime_error("PITA Slopes for IHWP-IN(OUT) not defined!\n\
		                          Add slope-ihwp-in(out) to the .conf file.");
	}
	NPatterns = 100;
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

	return 0;
}

void QwPitaFeedback::ProcessData()
{
	for (size_t i = 0; i < fDependentVar.size(); ++i) {
		// Call assignment operator to fill fOutputVar
		// (in our case: fOutputVar is a QwMollerADC_Channel)
		// This will fill our copied channels from which
		// we will accumulate our fRunningsum
    	fOutputVar[i]->AssignValueFrom(fDependentVar[i]);
		// fOutputVar[i]->PrintInfo();
	}

	/*
	for (size_t i = 0; i < fDependentValues.size(); ++i) {
		fDependentValues[i] = QwMollerADC_Channel->GetValue();
	}
	*/
	/*
	for (size_t i = 0; i < fDependentValues.size(); ++i) {
		fOutputValues.at(i) = fDependentValues[i];
	}
	for( auto const & out : fOutputValues )
		QwMessage << "fOutputValues = " << out << QwLog::endl;
	*/
}

// This is a hack. We should derive a FeedbackHandler from QwDataHandler
// and abstract this logic there. The re-direction scheme is error prone
// and hard to follow -- mrc (09/19/25)
void QwPitaFeedback::AccumulateRunningSum(VQwDataHandler &value, Int_t count, Int_t ErrorMask)
{
	// this = fRunningsum
	VQwDataHandler::AccumulateRunningSum(value, count, ErrorMask);
	// well this is horrible re-indirection
	if(GetGoodEventCount(0) >= NPatterns) {
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
	QwMessage << fRunningsum->GetValue(0)      / Qw::ppm << "\n\t";
	QwMessage << fRunningsum->GetValueError(0) / Qw::ppm << "\n\t";
	QwMessage << fRunningsum->GetValueWidth(0) / Qw::ppm << QwLog::endl;
	
	double correction = fRunningsum->GetValue(0) / Qw::ppm / slope->GetSlope();
	QwMessage << "Correction value = " << correction << QwLog::endl;
	
	double val = 0;
	setpoint1.Read(val);
	QwMessage << "setpoint1 value = " << val << QwLog::endl;
	setpoint2.Read(val);
	QwMessage << "setpoint2 value = " << val << QwLog::endl;
	setpoint3.Read(val);
	QwMessage << "setpoint3 value = " << val << QwLog::endl;
	setpoint4.Read(val);
	QwMessage << "setpoint4 value = " << val << QwLog::endl;
	setpoint5.Read(val);
	QwMessage << "setpoint5 value = " << val << QwLog::endl;
	setpoint6.Read(val);
	QwMessage << "setpoint6 value = " << val << QwLog::endl;
	setpoint7.Read(val);
	QwMessage << "setpoint7 value = " << val << QwLog::endl;
	setpoint8.Read(val);
	QwMessage << "setpoint8 value = " << val << QwLog::endl;

	// Which way does the sign go?
	// setpointN +- correction?
}
