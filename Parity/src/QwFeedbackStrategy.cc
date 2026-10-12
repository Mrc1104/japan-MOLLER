#include "QwFeedbackStrategy.h"
#define WAIT_FOR_TX_ACK

void QwPitaFBStrategy::Apply(double const correction, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	for(std::size_t index = 0; index < N; index++) {
		if(index < 4) {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::plus<double>>(correction) );
		} else {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::minus<double>>(correction) );
		}
	}
#ifdef WAIT_FOR_TX_ACK
	for( auto& tx : ack ) {
		tx.get();
	}
#endif
}


void QwAposUFBStrategy::Apply(double const correction, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	for(std::size_t index = 0; index < N; index++) {
		if(index % 2) {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::minus<double>>(correction) );
		} else {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::plus<double>>(correction) );
		}
	}

#ifdef WAIT_FOR_TX_ACK
	for( auto& tx : ack ) {
		tx.get();
	}
#endif
}


void QwAposVFBStrategy::Apply(double const correction, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	for(std::size_t index = 0; index < N; index++) {
		if(index % 2) {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::minus<double>>(correction) );
		} else {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::plus<double>>(correction) );
		}
	}

#ifdef WAIT_FOR_TX_ACK
	for( auto& tx : ack ) {
		tx.get();
	}
#endif
}


void QwIAFBStrategy::Apply(double const correction, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	for(std::size_t index = 0; index < N; index++) {
		ack[index] = std::move( setpoints[index].ApplyCorrection<std::minus<double>>(correction) );
	}

#ifdef WAIT_FOR_TX_ACK
	for( auto& tx : ack ) {
		tx.get();
	}
#endif
}
