#include "QwFeedbackHandler.h"
#include <string_view>
#include <string>
#include <limits>
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
	std::string feedback_type;
  	file.PopValue("impl", feedback_type);
  	file.PopValue("slope_ihwp_in", fFeedback->GetSlope(IHWP::kIN));
  	file.PopValue("slope_ihwp_out",fFeedback->GetSlope(IHWP::kOUT));
  	file.PopValue("patterns",fMaxPattern);
	fFeedback->ConfigureFeedbackType(feedback_type);
	
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
		std::cout << config << '\n';
		fFeedback->Configure(std::move(config));
	}

	return 0;
}

Int_t QwFeedbackHandler::ConnectChannels(QwSubsystemArrayParity& yield, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
{
	auto [data_type, device] = fFeedback->RequestTargetDevice();
	std::cout << "Requesting channel: " << device << '\n';
 	// which one do we want to use?
	// ReturnInternalValue or
	// RequestExternalPointer
    // channel = asym.ReturnInternalValue(TString(device));
    // channel = RequestExternalPointer(TString(device));
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
		std::cout << "Pattern " << fPatternCounter << ")\n";
		std::cout << "\tValue = " << fDeviceAccum->GetValue() << '\n';
		std::cout << "\tValueError = " << fDeviceAccum->GetValueError() << '\n';
		std::cout << "\tValueWidth = " << fDeviceAccum->GetValueWidth() << '\n';
		fPatternCounter++;
	}
	if(fPatternCounter >= fMaxPattern) {
		// Apply correction
		fDeviceAccum->CalculateRunningAverage();
		std::cout << "Pattern " << fPatternCounter << ")\n";
		std::cout << "\tValue = " << fDeviceAccum->GetValue() << '\n';
		std::cout << "\tValueError = " << fDeviceAccum->GetValueError() << '\n';
		std::cout << "\tValueWidth = " << fDeviceAccum->GetValueWidth() << '\n';
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

