#include "QwFeedbackImpl.h"


bool QwPitaFBStrategy::SetSetpoint(const char* pv_name, QwFeedbackLogger& logger)
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

void QwPitaFBStrategy::operator()(double const correction)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	for(std::size_t index = 0; index < N; index++) {
		if(index < 4) {
			ack[index] = std::move( fSetpoints[index].ApplyCorrection<std::plus<double>>(correction) );
		} else {
			ack[index] = std::move( fSetpoints[index].ApplyCorrection<std::minus<double>>(correction) );
		}
	}

	for( auto& tx : ack ) {
		tx.get();
	}
}


