#pragma once
#include <string_view>
#include <array>
#include "QwLog.h"
#include "QwFeedbackSetpoints.h"
// #include "VQwHardwareChannel.h"
class VQwEnumArray;
class VQwHardwareChannel;

struct QwPitaFBStrategy
{
	static constexpr std::string_view fName{"PITA Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{8};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(VQwHardwareChannel const* device, VQwEnumArray const* slope, SetPointArray& setpoints);
};

struct QwAposUFBStrategy
{
	static constexpr std::string_view fName{"AposU Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{4};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(VQwHardwareChannel const* device, VQwEnumArray const* slope, SetPointArray& setpoints);
};

struct QwAposVFBStrategy
{
	static constexpr std::string_view fName{"AposV Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{4};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(VQwHardwareChannel const* device, VQwEnumArray const* slope, SetPointArray& setpoints);
};


struct QwIAFBStrategy
{
	static constexpr std::string_view fName{"IA Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{4};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(VQwHardwareChannel const* device, VQwEnumArray const* slope, SetPointArray& setpoints);
};

struct QwPosXYFBStrategy
{
	static constexpr std::string_view fName{"PosXY Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{8};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(VQwHardwareChannel const* device, VQwEnumArray const* slope, SetPointArray& setpoints);
};

template <typename Strategy>
class QwFBStrategy
{
public:
	static constexpr std::string_view fName            = Strategy::fName;
	static constexpr std::size_t fNumSetpointsExpected = Strategy::fNumSetpointsExpected;
public:
	bool SetSetpoint(const char* pv_name, QwFeedbackLogger& logger);
	void operator()(VQwHardwareChannel const* device, VQwEnumArray const* slope);
private:
	std::size_t fNumSetpointsSet{0};
	std::array<QwFeedbackSetpoint, fNumSetpointsExpected> fSetpoints;
};

template <typename Strategy>
bool QwFBStrategy<Strategy>::SetSetpoint(const char* pv_name, QwFeedbackLogger& logger)
{
	if(fNumSetpointsSet >= fNumSetpointsExpected) {
		QwError << "Error: " << fName << " Setpoints already set!\n";
		return false;
	}
	QwMessage << "Adding Setpoint: " << pv_name << '\n';
	auto& setpoint = fSetpoints[fNumSetpointsSet++];
	setpoint.Attach(pv_name);
	setpoint.AddObserver(&logger);
	return true;
}

template <typename Strategy>
void QwFBStrategy<Strategy>::operator()(VQwHardwareChannel const* device, VQwEnumArray const* slope)
{
	if(fNumSetpointsSet == fNumSetpointsExpected) {
		Strategy::Apply(device, slope, fSetpoints);
	} else {
		QwError << fName << " does not have the expected number of setpoints set!"
		        << "\n\tNo. Set: "      << fNumSetpointsSet 
		        << "\n\tNo. Expected: " << fNumSetpointsExpected << QwLog::endl;
	}
}

// Helpful Aliases
using QwPitaStrategy  = QwFBStrategy<QwPitaFBStrategy>;
using QwAposUStrategy = QwFBStrategy<QwAposUFBStrategy>;
using QwAposVStrategy = QwFBStrategy<QwAposVFBStrategy>;
using QwIAStrategy    = QwFBStrategy<QwIAFBStrategy>;
using QwPosXYStrategy = QwFBStrategy<QwPosXYFBStrategy>;
