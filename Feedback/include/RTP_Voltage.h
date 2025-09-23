#ifndef __RTP_VOLTAGE_H
#define __RTP_VOLTAGE_H
#include <array>
#include <functional>
#include "QwEPICSControl.h"
#include "QwLog.h"

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
	RTP(std::string ioc_name, POLARITY polarity, HELICITY helicity)
	: channel(std::move(ioc_name))
	, pol(polarity)
	, hel(helicity) { }
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
};

template class RTP<double>;
template class RTP<int>;

#endif
