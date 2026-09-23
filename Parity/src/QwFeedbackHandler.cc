#include "QwFeedbackHandler.h"
#include <string_view>
QwFeedbackHandler::QwFeedbackHandler(TString const& name)
: VQwDataHandler(name)
, fFeedback{std::make_unique<QwFeedback>()}
{
  	fKeepRunningSum = true;
}
QwFeedbackHandler::QwFeedbackHandler(QwFeedbackHandler const& source)
: VQwDataHandler(source)
, fFeedback(std::make_unique<QwFeedback>(*source.fFeedback))
{ }


void QwFeedbackHandler::ParseConfigFile(QwParameterFile& file)
{
	VQwDataHandler::ParseConfigFile(file);
	std::string feedback_type;
  	file.PopValue("impl", feedback_type);
  	file.PopValue("slope_ihwp_in", fFeedback->GetSlope(IHWP::kIN));
  	file.PopValue("slope_ihwp_out",fFeedback->GetSlope(IHWP::kOUT));
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
    const VQwHardwareChannel* device_ptr{nullptr};
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
        device_ptr = yield.ReturnInternalValue(TString(device));
        break;
      case kHandleTypeAsym:
        SetEventcutErrorFlagPointer(asym.GetEventcutErrorFlagPointer());
        device_ptr = asym.ReturnInternalValue(TString(device));
        break;
      case kHandleTypeDiff:
        SetEventcutErrorFlagPointer(diff.GetEventcutErrorFlagPointer());
        device_ptr = diff.ReturnInternalValue(TString(device));
        break;
      default:
        QwWarning << "Warning: QwFeedbackHander::ConnectChannels():\n";
		QwWarning << "Unknown data type (" << data_type << ") for device (" << device << ")";
        QwWarning << QwLog::endl;
        break;
    }

	if(device_ptr) {
		std::cout << "not null!\n";
		fDependentVar.push_back(device_ptr);
		fOutputVar.push_back(device_ptr->Clone(VQwDataElement::kDerived));
	}
	else std::cout << "null\n";


	return 0;
}

