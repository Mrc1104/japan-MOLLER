#include "QwFeedbackImpl.h"
#include "QwFeedbackConfig.h"

QwPITAFeedback::QwPITAFeedback(QwFeedbackLogger& logger)
: fNumSetpointsSet{0}
, fDevice{}
, fLogger(logger)
{

}

std::unique_ptr<VQwFeedbackImpl> QwPITAFeedback::Clone() const
{
	return std::make_unique<QwPITAFeedback>( *this );
}

void QwPITAFeedback::ConfigureImpl(QwFeedbackConfig&& config)
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
void QwPITAFeedback::ApplyCorrectionImpl(double const correction)
{
	for(std::size_t index = 0; index < fPitaVoltages.size(); index++) {
		if(index < 4) {
			auto val = fPitaVoltages[index].ApplyCorrection<std::plus<double>>(correction);
		} else {
			auto val = fPitaVoltages[index].ApplyCorrection<std::minus<double>>(correction);
		}
	}
}

std::pair<VQwDataHandler::EQwHandleType, std::string_view>
QwPITAFeedback::RequestTargetDeviceImpl() const
{
	return {VQwDataHandler::EQwHandleType::kHandleTypeAsym, fDevice};
}


bool QwPITAFeedback::SetDeviceName(std::string&& name)
{
	if(!fDevice.empty()) {
		std::cout << "Warning: PITA Feedback Device name already set!\n";
		std::cout << "\tCurrent: " << fDevice << '\n';
		std::cout << "\tGiven:   " << name    << '\n';
		return false;
	}
	fDevice = std::move(name);
	return true;
}

// Need to connect to an IOC
bool QwPITAFeedback::AddSetpoint(std::string&& setp_name)
{
	if(fNumSetpointsSet >= fNumSetpointsExpected) {
		std::cout << "Error: PITA Feedback Setpoints already set!\n";
		return false;
	}
	std::cout << "Adding Setpoint: " << setp_name << '\n';
	auto& HV = fPitaVoltages[fNumSetpointsSet++];
	HV.Attach(setp_name.c_str());
	HV.AddObserver(&fLogger);

	return true;

}

