#include "QwFeedbackHandler.h"
QwFeedbackHandler::QwFeedbackHandler(TString const& name)
: VQwDataHandler(name)
{}
QwFeedbackHandler::QwFeedbackHandler(QwFeedbackHandler const& source)
: VQwDataHandler(source)
{ }

void QwFeedbackHandler::ParseConfigFile(QwParameterFile& file)
{
	VQwDataHandler::ParseConfigFile(file);
  	file.PopValue("slope_ihwp_in",fPrefix);
}
Int_t QwFeedbackHandler::LoadChannelMap(std::string const& mapfile)
{


}


