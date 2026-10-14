#pragma once
#include <string>
#include "QwFeedbackSetpoints.h"
#include "QwFeedbackLogger.h"
#include "QwFeedbackStrategy.h"
#include "QwFeedbackSlope.h"
#include "QwFeedbackCorrectionPayload.h"


class VQwFeedbackImpl
{
public:
	virtual void ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope) = 0;
	virtual std::unique_ptr<VQwFeedbackImpl> Clone() const = 0;
	virtual bool AddSetpoint(std::string const& setpoints) = 0;
};

template<typename FeedbackStrategy>
class QwFeedbackImpl : public VQwFeedbackImpl
{
	FeedbackStrategy fFbStrategy;
	QwFeedbackLogger& fLogger;
public:
	QwFeedbackImpl(QwFeedbackLogger& logger);
	void ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope) override;
	std::unique_ptr<VQwFeedbackImpl> Clone() const override;
	bool AddSetpoint(std::string const& setpoint) override;
};

template<typename FeedbackStrategy>
QwFeedbackImpl<FeedbackStrategy>::QwFeedbackImpl(QwFeedbackLogger& logger)
: fFbStrategy{}
, fLogger(logger)
{ }

template<typename FeedbackStrategy>
bool QwFeedbackImpl<FeedbackStrategy>::AddSetpoint(std::string const& setpoint)
{
	return fFbStrategy.SetSetpoint(setpoint.c_str(), fLogger);
}

template<typename FeedbackStrategy>
void QwFeedbackImpl<FeedbackStrategy>::ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope)
{
	fFbStrategy(payload, slope);
}


template<typename FeedbackStrategy>
std::unique_ptr<VQwFeedbackImpl> QwFeedbackImpl<FeedbackStrategy>::Clone() const
{
	return std::make_unique<QwFeedbackImpl>( *this );
}
