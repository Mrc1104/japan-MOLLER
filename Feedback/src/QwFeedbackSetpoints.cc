#include "EpicHandler.h"
#include "QwFeedbackSetpoints.h"




QwFeedbackSlope::QwFeedbackSlope() : fSlopes{1.0, 1.0}, fIHWP() {}
double& QwFeedbackSlope::operator[](IHWP state)       { return fSlopes[static_cast<int>(state)]; }
double  QwFeedbackSlope::operator[](IHWP state) const { return fSlopes[static_cast<int>(state)]; }
double  QwFeedbackSlope::GetSlope() const             { return this->operator[](fIHWP.GetState());}
void    QwFeedbackSlope::AttachLogger(QwFeedbackLogger& logger) { fIHWP.AddObserver(&logger); }



void QwFeedbackSetpoint::Update(double const& data)
{
	fPrev = fCurr.Exchange(data, std::memory_order_acq_rel);
	std::string log_payload = "Received: " + fName
	                            + ": fPrev = "        + std::to_string(fPrev)
		                        + ", fCurrent = "     + std::to_string(data)
								+ " => Correction = " + std::to_string(fPrev - data) + '\n';
	Notify(QwFeedbackLogPayload{QwLogLevel::kMessage, std::move(log_payload)}); 
}

void QwFeedbackSetpoint::Attach(const char* pv_name)
{
	if(fChannel) fChannel->StopMonitoring(this);
	fChannel = EpicHandler::Instance().ConnectChannel(pv_name);
	fChannel->StartMonitoring(this);
	fName = fChannel->GetName();
}


QwFeedbackSetpoint::QwFeedbackSetpoint()
: fChannel(nullptr)
, fCorr{}
, fPrev{}
, fCurr{}
{}

QwFeedbackSetpoint::QwFeedbackSetpoint(EpicChannel* channel)
: fChannel(channel)
, fCorr{}
, fPrev{}
, fCurr{}
{
	if(channel) {
		fChannel->StartMonitoring(this);
	}
}

QwFeedbackSetpoint::~QwFeedbackSetpoint()
{
	if(fChannel) {
		fChannel->StopMonitoring(this);
	}
}

QwFeedbackSetpoint::QwFeedbackSetpoint(QwFeedbackSetpoint const& other)
: fChannel(other.fChannel)
{
	double current = other.fCurr.Load(std::memory_order_acquire);
	fCorr = other.fCorr;
	fPrev = other.fPrev;
	fCurr.Store(current, std::memory_order_release);
}


