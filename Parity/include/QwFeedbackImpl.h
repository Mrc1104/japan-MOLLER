#pragma once
#include <array>
#include <functional>
#include <string>
#include "QwFeedbackSetpoints.h"
#include "VQwDataHandler.h" // Access EQwHandleType
#include "QwFeedbackLogger.h"
#include "QwFeedbackConfig.h"
#include "QwFeedbackStrategy.h"
#include "QwFeedbackSlope.h"
#include "QwFeedbackCorrectionPayload.h"


class VQwHardwareChannel;

class VQwFeedbackImpl
{
public:
	virtual void ConfigureImpl(QwFeedbackConfig&& config)        = 0;
	virtual void ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope) = 0;
	virtual std::pair<VQwDataHandler::EQwHandleType, std::string_view>
	RequestTargetDeviceImpl() const    = 0;
	virtual std::unique_ptr<VQwFeedbackImpl> Clone() const = 0;
};

template<typename FeedbackStrategy>
class QwFeedbackImpl : public VQwFeedbackImpl
{
	FeedbackStrategy fFbStrategy;
	std::string fDevice;
	QwFeedbackLogger& fLogger;
private:
	bool SetDeviceName(std::string&& name);
	bool AddSetpoint(std::string&& setp_name);
public:
	QwFeedbackImpl(QwFeedbackLogger& logger);
	void ConfigureImpl(QwFeedbackConfig&& config) override;
	void ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope) override;
	std::pair<VQwDataHandler::EQwHandleType, std::string_view>
	RequestTargetDeviceImpl() const override;
	std::unique_ptr<VQwFeedbackImpl> Clone() const override;
};

template<typename FeedbackStrategy>
bool QwFeedbackImpl<FeedbackStrategy>::SetDeviceName(std::string&& name)
{
	if(!fDevice.empty()) {
		QwWarning << "Warning: PITA Feedback Device name already set!\n"
		          << "\tCurrent: " << fDevice << '\n'
		          << "\tGiven:   " << name    << '\n';
		return false;
	}
	fDevice = std::move(name);
	return true;
}

template<typename FeedbackStrategy>
bool QwFeedbackImpl<FeedbackStrategy>::AddSetpoint(std::string&& setp_name)
{
	return fFbStrategy.SetSetpoint(setp_name.c_str(), fLogger);
}

template<typename FeedbackStrategy>
QwFeedbackImpl<FeedbackStrategy>::QwFeedbackImpl(QwFeedbackLogger& logger)
: fFbStrategy{}
, fDevice{}
, fLogger(logger)
{ }

template<typename FeedbackStrategy>
void QwFeedbackImpl<FeedbackStrategy>::ConfigureImpl(QwFeedbackConfig&& config)
{
	switch(config.getType()) {
		case QwFeedbackConfig::TYPE::kTARGET:
			SetDeviceName(std::move(config).getName());
			break;
		case QwFeedbackConfig::TYPE::kIOC:
			AddSetpoint(std::move(config).getName());
			break;
		default:
			break;
	}
}

template<typename FeedbackStrategy>
void QwFeedbackImpl<FeedbackStrategy>::ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope)
{
	fFbStrategy(payload, slope);
}


template<typename FeedbackStrategy>
std::pair<VQwDataHandler::EQwHandleType, std::string_view>
QwFeedbackImpl<FeedbackStrategy>::RequestTargetDeviceImpl() const
{
	return {VQwDataHandler::EQwHandleType::kHandleTypeAsym, fDevice};
}

template<typename FeedbackStrategy>
std::unique_ptr<VQwFeedbackImpl> QwFeedbackImpl<FeedbackStrategy>::Clone() const
{
	return std::make_unique<QwFeedbackImpl>( *this );
}
