#pragma once
#include <array>
#include <functional>
#include <string>
#include "QwFeedbackSetpoints.h"
#include "VQwDataHandler.h" // Access EQwHandleType
#include "QwFeedbackLogger.h"

class QwFeedbackConfig;

class VQwFeedbackImpl
{
public:
	virtual void ConfigureImpl(QwFeedbackConfig&& config)        = 0;
	virtual void ApplyCorrectionImpl(double const running_average) = 0;
	virtual std::pair<VQwDataHandler::EQwHandleType, std::string_view>
	RequestTargetDeviceImpl() const    = 0;
	virtual std::unique_ptr<VQwFeedbackImpl> Clone() const = 0;
};

class QwPITAFeedback : public VQwFeedbackImpl
{
	// Maybe use std::variant?
	using ADD = std::plus<double>;
	using SUB = std::minus<double>;
	static constexpr std::size_t fNumSetpointsExpected{8};
	std::array<QwFeedbackSetpoint, fNumSetpointsExpected> fPitaVoltages;
	std::size_t fNumSetpointsSet;
	std::string fDevice;
	QwFeedbackLogger& fLogger;
private:
	bool SetDeviceName(std::string&& name);
	bool AddSetpoint(std::string&& setp_name);
public:
	QwPITAFeedback(QwFeedbackLogger& logger);
	void ConfigureImpl(QwFeedbackConfig&& config) override;
	void ApplyCorrectionImpl(double const running_average) override;
	std::pair<VQwDataHandler::EQwHandleType, std::string_view>
	RequestTargetDeviceImpl() const override;
	std::unique_ptr<VQwFeedbackImpl> Clone() const override;
};

