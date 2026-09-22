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

QwFeedbackHandler::~QwFeedbackHandler() = default;


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

Int_t QwFeedbackHandler::ConnectChannels(QwSubsystemArrayParity& /*yield*/, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
{
	return 0;
}

