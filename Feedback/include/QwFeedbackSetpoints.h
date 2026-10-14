#pragma once
#include <future>
#include <string>
#include "QwFeedbackLogger.h"
#include "QwEpicTypes.h"
#include "QwEpicChannel.h"
#include "QwIHWP.h"
#include "QwLimits.h"

class QwFeedbackSetpoint : public EPICSObserver<double>, public Publisher<QwFeedbackLogPayload>
{
private:
	EpicChannel* fChannel;
	double fCorr;
	double fPrev;
	QwSetpointLimits fLimits;
	AtomicEpicsType<double> fCurr;
	std::string fName;

	bool IsWithinLimits(double const input) const noexcept;
public:
	void Attach(const char* pv_name);
	void SetLimits(QwSetpointLimits const& limits);
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
	if(!fChannel) {
		std::string log_payload = "Attempting to send request to "
			+ fName
			+ ", but is null... something bad has happened!\n";
		Notify(QwFeedbackLogPayload{QwLogLevel::kError, std::move(log_payload)});
		return std::future<void>{};
	}

	double current = fCurr.Load(std::memory_order_acquire);
	do {
		fCorr = BinaryOp{}(current, corr);
		if(IsWithinLimits(fCorr) == false) {
			std::string log_payload = "Request: " + fName
				+ ": fPrev = "        + std::to_string(fPrev)
				+ ", fCurrent = "     + std::to_string(current)
				+ " => Correction = " + std::to_string(fCorr)
				+ " is outside of limits [" + std::to_string(fLimits.lower_limit)
				+ ", " + std::to_string(fLimits.upper_limit) + "]\n";
			Notify(QwFeedbackLogPayload{QwLogLevel::kError, std::move(log_payload)});
			return std::future<void>{};
		}
	} while(!fCurr.CompareExchangeWeak(current, fCorr,
				std::memory_order_release, std::memory_order_acquire));
	auto fut = fChannel->PutAsync(DBR_DOUBLE, current);
	std::string log_payload = "Request: " + fName
                            + ": fPrev = "        + std::to_string(fPrev)
	                        + ", fCurrent = "     + std::to_string(current)
							+ " => Correction = " + std::to_string(fCorr) + '\n';
	Notify(QwFeedbackLogPayload{QwLogLevel::kMessage, std::move(log_payload)});
	return fut;
}

