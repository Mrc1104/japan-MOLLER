#include "QwFeedback.h"
#include "QwFeedbackImpl.h"
#include "QwFeedbackConfig.h"

QwFeedback::QwFeedback()
: fPimpl(nullptr)
, fSlope{}
{}

QwFeedback::QwFeedback(QwFeedback const& other)
: fPimpl( other.fPimpl ? other.fPimpl->Clone() : nullptr )
, fSlope(other.fSlope)
, fLogFile{}
{
	
}


void QwFeedback::ConfigureFeedbackType(std::string_view sv)
{
	if(sv.find("PITA") != std::string_view::npos) {
		ConfigureFeedbackType(TYPE::PITA);
	}
}
void QwFeedback::ConfigureFeedbackType(TYPE type)
{
	if(fPimpl) std::cout << "WARNING: FeedbackType is already configured! Ignoring...\n";
	switch(type) {
		case TYPE::PITA:
			fPimpl = std::make_unique<QwPITAFeedback>(fLogFile);
			break;
		default:
			break;
	}
	return;
}

bool QwFeedback::ConfigureLogger(std::string const& str, std::ios_base::openmode mode)
{
	fLogFile.SetSink(str);
	return static_cast<bool>(fLogFile);
}

void    QwFeedback::SetSlope(IHWP state, double val) { fSlope[state] = val ; }
double  QwFeedback::GetSlope(IHWP state) const       { return fSlope[state]; }
double& QwFeedback::GetSlope(IHWP state)             { return fSlope[state]; }
double  QwFeedback::GetSlope() const                 { return fSlope.GetSlope(); }

void QwFeedback::ConfigureFeedback(QwFeedbackConfig&& config)
{
	if(fPimpl) fPimpl->ConfigureImpl(std::move(config));
}

void QwFeedback::ApplyCorrection(double const running_average)
{
	if(fPimpl) fPimpl->ApplyCorrectionImpl(running_average / GetSlope());
}


std::pair<VQwDataHandler::EQwHandleType, std::string_view>
QwFeedback::RequestTargetDevice() const
{
	return fPimpl
		   ? fPimpl->RequestTargetDeviceImpl()
		   : std::pair{VQwDataHandler::kHandleTypeUnknown, std::string_view{}};
}

