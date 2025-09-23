#ifndef __RTP_VOLTAGE_H
#define __RTP_VOLTAGE_H
#include <array>
#include <functional>
#include "QwEPICSControl.h"

/* I need to know the IOC, the polarity, and the helicity */
enum class POLARITY : int
{
	NEGATIVE,
	POSITIVE,
	UNKNOWN
};
typedef POLARITY HELICITY;

template<typename T>
class RTP
{
public:
	RTP(std::string ioc_name, POLARITY polarity, HELICITY helicity/*, CorrectionStrategy strategy*/)
	: channel(std::move(ioc_name))
	, pol(polarity)
	, hel(helicity) { }
	// , correction(strategy) { }
public:
	POLARITY GetPolarization()const { return pol; }
	HELICITY GetHelicity()    const { return hel; }
	T GetChannelValue()
	{
		T val = 0;
		channel.Read(val);
		channel.Sync();
		return val;
	}
	void SetChannelValue(const T val)
	{
		channel.Write(val);
		channel.Sync();
	}
private:
	QwEPICSChannel<T> channel;
	POLARITY pol;
	HELICITY hel;
	// CorrectionStrategy correction;
};

template class RTP<double>;
template class RTP<int>;
	
// I will need at least three CalcCorrection functions to support
// all of the feedbacks
// 	1) asymmetric (PITA)
//		-- Increasing the volage in one helicity state while
//		-- decreasing the voltage in the other helicity state
//	2) symmetric (IA)
//		-- Increase or Decrease all voltages equally
//  3) psuedo-XOR
//		-- If Hel == Pol, subtract
//		-- If Hel != Pol, add
// Create a std::function pointer for the different calc routines (could
// do a templated function ptr thing)
inline void asymmetric_correction(RTP<double>& setpoint, double correction)
{
	// 	1) asymmetric (PITA)
	//		-- Increasing the volage in one helicity state (neg) while
	//		-- decreasing the voltage in the other helicity state (pos)
	int sign = (setpoint.GetPolarization() == POLARITY::POSITIVE) ? -1 : +1;
	double curr_setpoint = setpoint.GetChannelValue();
	setpoint.SetChannelValue(curr_setpoint + sign * correction);
}
inline void symmetric_correction(RTP<double>& setpoint, double correction)
{
	//	2) symmetric (IA)
	//		-- Increase or Decrease all voltages equally
	int sign = -1;
	double curr_setpoint = setpoint.GetChannelValue();
	setpoint.SetChannelValue(curr_setpoint + sign * correction);
}
inline void hel_pol_dependent_correction(RTP<double>& setpoint, double correction)
{
	//  3) psuedo-XOR
	//		-- If Hel == Pol, subtract
	//		-- If Hel != Pol, add
	int sign = (setpoint.GetPolarization() == setpoint.GetHelicity()) ? -1 : +1;
	double curr_setpoint = setpoint.GetChannelValue();
	setpoint.SetChannelValue(curr_setpoint + sign * correction);
}
inline std::array<std::function<void(RTP<double>&, double)>, 3> CorrectionRoutines {
	asymmetric_correction,
	symmetric_correction,
	hel_pol_dependent_correction
};
#endif
