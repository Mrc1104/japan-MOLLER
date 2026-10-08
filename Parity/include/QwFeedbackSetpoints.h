#pragma once
#include <future>
#include <string>
#include "QwFeedbackLogger.h"
#include "EpicTypes.h"
#include "EpicChannel.h"
#include "IHWP.h"

class QwFeedbackSlope
{
	static_assert( static_cast<int>(IHWP::kIN) == 0
			&& static_cast<int>(IHWP::kOUT) == 1,
			"Expected: enum IHWP is used for indexing!\n");
	std::array<double, 2> fSlopes;
	IHWP_IOC fIHWP;
public:
	QwFeedbackSlope();
	double& operator[](IHWP state);
	double  operator[](IHWP state) const;
	double  GetSlope() const;
	void    AttachLogger(QwFeedbackLogger& logger);
};


class QwFeedbackSetpoint : public EPICSObserver<double>, public Publisher<QwFeedbackLogPayload>
{
private:
	EpicChannel* fChannel;
	double fCorr;
	double fPrev;
	AtomicEpicsType<double> fCurr;
	std::string fName;
public:
	void Attach(const char* pv_name);
	void Update(double const& data) override;
	template<typename BinaryOp>
	std::future<void> ApplyCorrection(double corr);
public:
	QwFeedbackSetpoint();
	QwFeedbackSetpoint(EpicChannel* channel);
	~QwFeedbackSetpoint();
	QwFeedbackSetpoint(QwFeedbackSetpoint const& other);
	QwFeedbackSetpoint(QwFeedbackSetpoint&& other) = delete;
	QwFeedbackSetpoint& operator=(QwFeedbackSetpoint const& other) = delete;
	QwFeedbackSetpoint& operator=(QwFeedbackSetpoint&& other) = delete;
};

template<typename BinaryOp>
std::future<void> QwFeedbackSetpoint::ApplyCorrection(double corr)
{
	double current = fCurr.Load(std::memory_order_acquire);
	do {
		fCorr = BinaryOp{}(current, corr);
	} while(!fCurr.CompareExchangeWeak(current, fCorr,
				std::memory_order_release, std::memory_order_acquire));
	if(fChannel) {
		auto fut = fChannel->PutAsync(DBR_DOUBLE, current);
		std::string log_payload = "Request: " + fName
	                            + ": fPrev = "        + std::to_string(fPrev)
		                        + ", fCurrent = "     + std::to_string(current)
								+ " => Correction = " + std::to_string(fCorr);
		Notify(QwFeedbackLogPayload{QwLogLevel::kMessage, std::move(log_payload)}); 
		return fut;
	}
	std::string log_payload = "Attempting to send request to "
							  + fName
							  + ", but is null... something bad has happened!";
	Notify(QwFeedbackLogPayload{QwLogLevel::kError, std::move(log_payload)});
	return std::future<void>{};
}

