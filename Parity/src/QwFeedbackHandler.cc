#include "QwFeedbackHandler.h"
#include <string_view>
#include <string>
#include <limits>
#include "QwFeedbackConfig.h"
#include "QwFeedbackImpl.h"

QwFeedbackHandler::QwFeedbackHandler(TString const& name)
: VQwDataHandler(name)
, fMaxPattern(std::numeric_limits<std::size_t>::max())
, fPatternCounter(0)
, fDeviceObserver{nullptr}
, fDeviceAccum{nullptr}
, fFeedback{std::make_unique<QwFeedback>()}
{
	fKeepRunningSum = false;
}
QwFeedbackHandler::QwFeedbackHandler(QwFeedbackHandler const& source)
: VQwDataHandler(source)
, fMaxPattern(source.fMaxPattern)
, fPatternCounter(source.fPatternCounter)
, fDeviceObserver(source.fDeviceObserver)
, fDeviceAccum( source.fDeviceAccum->Clone(VQwDataElement::kDerived) )
, fFeedback(std::make_unique<QwFeedback>(*source.fFeedback))
{ }


void QwFeedbackHandler::ParseConfigFile(QwParameterFile& file)
{
	VQwDataHandler::ParseConfigFile(file);
	std::string feedback_type, logfile;

	// Instantiate the feedback type first before setting slopes
  	file.PopValue("impl", feedback_type); 
	auto feedback_type_enum = fFeedback->ConfigureFeedbackType(feedback_type);
	if(feedback_type_enum != QwFeedback::TYPE::UNKNOWN) {
		THROW_ERROR("Unsupported Feedback Type: " + std::string(feedback_type));
	}
	if(feedback_type_enum != QwFeedback::TYPE::POSXY) {
		double slope_in = 0, slope_out = 0;
		if(!file.PopValue("slope_ihwp_in", slope_in)) {
			THROW_ERROR("Missing Required Slope (slope_ihwp_in)!");
		}
		if(!file.PopValue("slope_ihwp_out", slope_out)) {
			THROW_ERROR("Missing Required Slope (slope_ihwp_out)!");
		}
		fFeedback->SetSlope(IHWP::kIN , slope_in );
		fFeedback->SetSlope(IHWP::kOUT, slope_out);
	} else {
		double slope_xu = 0, slope_xv = 0,
		       slope_yu = 0, slope_yv = 0;
		if(!file.PopValue("slope_xu", slope_xu)) {
			THROW_ERROR("Missing Required Slope (slope_xu)!");
		}
		if(!file.PopValue("slope_xv", slope_xv)) {
			THROW_ERROR("Missing Required Slope (slope_xv)!");
		}
		if(!file.PopValue("slope_yu", slope_yu)) {
			THROW_ERROR("Missing Required Slope (slope_yu)!");
		}
		if(!file.PopValue("slope_yu", slope_yv)) {
			THROW_ERROR("Missing Required Slope (slope_yv)!");
		}
		fFeedback->SetSlope(QwPOSXYSlope::POS_TYPE::kXU , slope_xu);
		fFeedback->SetSlope(QwPOSXYSlope::POS_TYPE::kXV , slope_xv);
		fFeedback->SetSlope(QwPOSXYSlope::POS_TYPE::kYU , slope_yu);
		fFeedback->SetSlope(QwPOSXYSlope::POS_TYPE::kYV , slope_yv);
	}

  	file.PopValue("patterns",fMaxPattern);
	file.PopValue("log", logfile);
	if( !fFeedback->ConfigureLogger(logfile) ) {
		QwWarning << '(' << logfile << ") Logfile Not Set for "
		          << feedback_type << " Feedback!" << QwLog::endl;
	}
	
}
Int_t QwFeedbackHandler::LoadChannelMap(std::string const& mapfile)
{
	std::cout << "Opening: " << mapfile<< '\n';
	// Open the file
	QwParameterFile map(mapfile);

	while (map.ReadNextLine()) {
		std::cout << "ReadNextLine()\n";
		// Throw away comments, whitespace, empty lines
		map.TrimComment();
		map.TrimWhitespace();
		if (map.LineIsEmpty()) continue;
		std::string token = map.GetNextToken(" ");
		QwFeedbackConfig config;
		while ( !token.empty() ) {
			config.parse_pair(token);
			token = map.GetNextToken(" ");
		}
		if(!config.isValid()) {
			QwWarning << "Invalid Feedback Config:" << config << '\n';
			continue;
		}
		QwOut << config << '\n';
		fFeedback->ConfigureFeedback(std::move(config));
	}

	return 0;
}

Int_t QwFeedbackHandler::ConnectChannels(QwSubsystemArrayParity& yield, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
{
	auto [data_type, device] = fFeedback->RequestTargetDevice();
	std::cout << "Requesting channel: " << device << '\n';
    switch (data_type) {
      case kHandleTypeYield:
        SetEventcutErrorFlagPointer(yield.GetEventcutErrorFlagPointer());
        fDeviceObserver = yield.ReturnInternalValue(TString(device));
        break;
      case kHandleTypeAsym:
        SetEventcutErrorFlagPointer(asym.GetEventcutErrorFlagPointer());
        fDeviceObserver = asym.ReturnInternalValue(TString(device));
        break;
      case kHandleTypeDiff:
        SetEventcutErrorFlagPointer(diff.GetEventcutErrorFlagPointer());
        fDeviceObserver = diff.ReturnInternalValue(TString(device));
        break;
      default:
        QwWarning << "Warning: QwFeedbackHander::ConnectChannels():\n";
		QwWarning << "Unknown data type (" << data_type << ") for device (" << device << ")";
        QwWarning << QwLog::endl;
        break;
    }

	if(fDeviceObserver == nullptr) {
		std::string msg = "Feedback Target Device (" 
					      + std::string(device)
						  + ") Not Found";
		throw std::runtime_error(std::move(msg));
	}
	fDeviceAccum =  fDeviceObserver->Clone(VQwDataElement::kDerived);


	return 0;
}

void QwFeedbackHandler::ProcessData()
{
	// Perform accumulation here
	// VQwDataHandler::AccumulateRunningSum is
	//    a) Not virtual and
	//    b) unergonomic (nested class structure)
	if(GetEventcutErrorFlag() == 0) {
		fDeviceAccum->AccumulateRunningSum(fDeviceObserver);
		fPatternCounter++;
	}
	if(fPatternCounter >= fMaxPattern) {
		// Apply correction
		fDeviceAccum->CalculateRunningAverage();
		fFeedback->ApplyCorrection(fDeviceAccum);
		QwOut << "Pattern " << fPatternCounter << ")\n";
		QwOut << "\tValue = " << fDeviceAccum->GetValue() * 1e6 << " [ppm]\n";
		QwOut << "\tValueError = " << fDeviceAccum->GetValueError() * 1e6 << " [ppm]\n";
		QwOut << "\tValueWidth = " << fDeviceAccum->GetValueWidth() * 1e6 << " [ppm]\n";
		fPatternCounter = 0;
	}
}
void QwFeedbackHandler::FinishDataHandler()
{
	ClearEventData();
}

void QwFeedbackHandler::ClearEventData()
{
	fDeviceAccum->ClearEventData();
}


void QwFeedbackHandler::ConstructTreeBranches( QwRootFile *treerootfile, const std::string& treeprefix, const std::string& branchprefix)
{
	// No-op
	return;
}
void QwFeedbackHandler::FillTreeBranches(QwRootFile *treerootfile)
{
	// No-op
	return;
}
void QwFeedbackHandler::ConstructNTupleFields( QwRootFile *treerootfile, const std::string& treeprefix, const std::string& branchprefix)
{
	// No-op
	return;
}
void QwFeedbackHandler::FillNTupleFields(QwRootFile *treerootfile)
{
	// No-op
	return;
}

void  QwFeedbackHandler::ConstructHistograms(TDirectory * /*folder*/, TString & /*prefix*/)
{
	// No-op
	return;
}
void  QwFeedbackHandler::FillHistograms()
{
	// No-op
	return;
}

