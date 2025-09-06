#ifndef __QW_ROTATABLE_HALF_WAVE_PLATE__
#define __QW_ROTATABLE_HALF_WAVE_PLATE__
#include "QwHalfWavePlate.h"

#include <string>

// The RHWP can take values from [0,8000]arb -> [0,360]deg (continuous)
// EPICS DB stores the arb units. Users can access the values in arb units
// by invoking GetStatus(), or get the converted values by calling GetStatus_[UNIT]();
class RHWP : public QwHalfWavePlate<double>
{
public:
	RHWP();
	RHWP(std::string ioc);

	double GetStatus_Deg() const;
	double GetStatus_Rad() const;
	double GetStatus_Arb() const; // For completeness
};


#endif
