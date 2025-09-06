#ifndef __QW_INSERTABLE_HALF_WAVE_PLATE__
#define __QW_INSERTABLE_HALF_WAVE_PLATE__
#include "QwHalfWavePlate.h"

#include <string>

/*
* QwInsertableHalfWavePlate
*	Contains the logic for the IHWP. The IHWP can have discrete values,
*	either 'IN' or 'OUT'. This is represented as a 1 or 0 in the EPICS
*	DB. For expressiveness, an enum class IHWP_STATUS is defined and
*	comparison operators are overloaded to allow for statements such as
*		if( ihwp == IHWP_STATUS::IN ) { ... }
*	And private update_status routine ensures that the status member is
*	always an enumerated value (represented as an int)
*
* Discussion:
*	We have an option fHalfWaveRevert, which if true, inverts the ihwp
*	status ('In'->'Out' and 'Out->'In'). Do we want to handle that in
*	the class?
*		Idea: Pass bool IHWP_Revert into the ctor and then invert
*		the value returned when GetStatus() is invoked.
*	Or, do we want to caller to handle any inversions? I don't think adding
*	a full QwOptions routine is necessary, but that us another method we
*	could use.
*/

enum class IHWP_STATUS : int
{
	OUT = 0,
	IN,
	UNKNOWN
};

class IHWP : public QwHalfWavePlate<int> // 'In' or 'Out' (Discrete)
{
public:
	IHWP();
	IHWP(std::string ioc);

	void Update()         override;
	// It would be nice if I could return
	// a IHWP_STATUS
	// int GetStatus() const override;
	IHWP_STATUS GetIHWPStatus() const;

	bool operator==(const IHWP_STATUS ihwp_status);
	bool operator!=(const IHWP_STATUS ihwp_status);
private:
	// Guarauntees that status corresponds to an
	// enumerated type
	void update_status(const int value);
};

#endif
