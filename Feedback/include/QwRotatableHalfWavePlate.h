#ifndef __QW_ROTATABLE_HALF_WAVE_PLATE__
#define __QW_ROTATABLE_HALF_WAVE_PLATE__
#include "QwHalfWavePlate.h"

#include <string>
/*
* QwRotatableHalfWavePlate
*	Contains the logic for the RHWP. The RHWP can take continuous values,
*	ranging from [0,8000] in micro-controller steps, which correspond to
*	[0,2pi]. The class offers routines to convert from the EPICS DB 'arb'
* 	units to either degrees or radians:
*		GetStatus_Deg() const;
*		GetStatus_Rad() const;
*		GetStatus_Arb() const; // For completeness, simply calls GetStatus()
*
* Discussion:
*/
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
