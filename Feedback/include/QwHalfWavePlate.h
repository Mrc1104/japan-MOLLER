#ifndef __QW_HALF_WAVE_PLATE__
#define __QW_HALF_WAVE_PLATE__


/*
 * QwHalfWavePlate
 *	This class contains the logic for the IHWPs located upstream of the pockel cells
 *  THere are two such HWPs: nonconfusingly named ihwp & ihwp2 in the original code.
 *  This class should contain:
 *		- DefineOptions(QwOptions&)
 *		---- The caller should be handling the options, not this class
 *		-- fAutoIWHP = allows the state to change as the feedback progress (what timescales?)
 *		---- This option is irrelevent to the class. Caller should handle this
 *		-- fHalfwaveRevert = Sets 'IN'->'OUT', 'OUT'->'IN'
 *		---- Could add this to QwInsertableHalfWavePlate as a Constructor param. All it would do is
 *		---- invert the return value on GetStatus() calls -- mrc (09/06/25)
 *      -- The IWP IOC (we'll use the QwOption default args to set the nominal IOC id)
 *		---- IGL1I00DI24_24M (ihwp)
 *		---- IGL1I00DIOFLRD  (ihwp2) this is a passive ihwp readback: (13056=IN,8960=OUT)
 *		------ (hmm... on second thought, default args won't work if there are two of them)
 *		------ This second halfwave plate does not seem to exist anymore ..? Could not find it in the epics arhcive.
 *		------ It also might make more sense to leave it up to the application to handle these options and rather have 
 *		------ a static char const * DEFAULT_IOC_NAME instead
 *	Desired Methods Public:
 *		- Status() = returns and enum with the hwp status
 *	Desired Methods Private:
 *		- Update() = reads the EPICS DB and updates the status
 *
 * Discussion:
 *	Feedback does not use the RHWP, do we need to support it with this class?
 *	If yes, then we would need to rethink how this class would work (because as it stands
 *	the class expetcs discrete states, not continuous (e.g. 'IN' or 'OUT', not 0-360 degrees)).
 *	
 *	If we wanted to unify RHWP and IHWP, then I'd suggest extending QwEPICSControl to support
 *	unsigned ints (as of 09/03/25, it only supports DBR_STRING and DBR_DOUBLE), that way we could
 *	use template specialization to handle the discrete and continuous case in one class.
 *	(when I have a hammer, all I see are nails -- mrc).
 *
 *	... How to handle the return type ..?
 *
 *  Using template class to achieve IWHP and RHWP functionality would likely be a huge pain
 *	they are only similar in name and not use. If we decide we need to support RHWP, then we should
 *	rename this class to IHWP, and make a seperate RHWP class.
 *	Or, keep QwHalfWavePlate name as is and make it a base class from which IWHP and RHWP could be
 *  derived. Options... options... -- mrc (09/05/25)
 *
 * Imagine:
 *	template< typename T >
 *	QwHalfWavePlate
 *	{
 *	public:
 *		Update();
 *		T GetStatus();
 *
 *	private:
 *		T status;
 *		QwEPICS<T> HWP;
 * }
 * Then we would derive it as
 * 	class QwRotatingWavePlate   : public QwHalfWavePlate<double> (continuous values)
 * 	class QwInsertableWavePlate : public QwHalfWavePlate<int> (discrete values)
 * and then we can define (outside of QwInsertableWavePlate), an enum with 'In' or 'OUT'
 * I really like this idea
 */

#include <string>

#include "QwEPICSControl.h"

template<typename T>
class QwHalfWavePlate
{
public:
	QwHalfWavePlate(std::string ioc);
	// virtual ~QwHalfwavePlate(); // Do we need this? How to make a templated virtual dtor?

	// Do we have a use case for a unified
	// "update and return new status functionality"?
	virtual void Update();
	virtual T GetStatus() const;

protected:
	QwEPICS<T> hwp_ioc;
	T status;
};
#endif
