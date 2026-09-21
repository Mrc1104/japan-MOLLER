#include "QwFeedbackHandler.h"
QwFeedbackHandler::QwFeedbackHandler(TString const& name)
: VQwDataHandler(name)
, fSlope{}
{}
QwFeedbackHandler::QwFeedbackHandler(QwFeedbackHandler const& source)
: VQwDataHandler(source)
, fSlope{}
{ }

void QwFeedbackHandler::ParseConfigFile(QwParameterFile& file)
{
	VQwDataHandler::ParseConfigFile(file);
  	file.PopValue("slope_ihwp_in", fSlope[IHWP::kIN]);
  	file.PopValue("slope_ihwp_out",fSlope[IHWP::kOUT]);
}
Int_t QwFeedbackHandler::LoadChannelMap(std::string const& mapfile)
{

	return 0;
}

