#include "QwHalfWavePlate.h"

static char const *DEFAULT_IOC_NAME = "IGL1I00DI24_24M";
HWP::HWP()
{
	// I do not like this -- mrc
	wave_plate(DEFAULT_IOC_NAME);
	status = HWP_STATUS::UNKNOWN:
}
HWP::HWP(std::string ioc)
{
	wave_plate(std::move(ioc));
	status = HWP_STATUS::UNKNOWN:
}


void HWP::Update()
{
	double value = 0.0;
	wave_plate.read(&value);
	update_status(value);
}

HWP_STATUS HWP::GetStatus()
{
	return status;
}

void HWP::update_status(double value)
{
	HWP_STATUS hwp_value = static_cast<HWP_STATUS>(value);
	switch(hwp_value) {
	case HWP_STATUS::OUT:
		status = HWP_STATUS::OUT;
	break;
	case HWP_STATUS::IN:
		status = HWP_STATUS::IN;
	break;
	default:
		status = HWP_STATUS::UNKNOWN;
		QwError << "HWP Status is HWP_STATUS::UNKNOWN" << QwLog::endl;
	break;
	}
}
