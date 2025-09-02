#ifndef __QW_HALF_WAVE_PLATE__
#define __QW_HALF_WAVE_PLATE__


/*
 * QwHalfWavePlate
 *	This class contains the logic for the IHWPs located upstream of the pockel cells
 *  THere are two such HWPs: nonconfusingly named ihwp & ihwp2 in the original code.
 *  This class should contain:
 *		- DefineOptions(QwOptions&)
 *		-- fAutoIWHP = allows the state to change as the feedback progress (what timescales?)
 *		-- fHalfwaveRevert = Sets 'IN'->'OUT', 'OUT'->'IN'
 *      -- The IWP IOC (we'll use the QwOption default args to set the nominal IOC id)
 *		---- IGL1I00DI24_24M (ihwp)
 *		---- IGL1I00DIOFLRD  (ihwp2) (hmm... on second thought, default args won't work if there are two of them)
 *	Desired Methods Public:
 *		- Status() = returns and enum with the hwp status
 *		-- enum hwp_state{ IN, OUT, UNKNOWN}
 *	Desired Methods Private:
 *		- Update() = reads the EPICS DB and updates the status
 *
 * Discussion:
 */

enum class HWP_STATUS
{
	IN,
	OUT,
	UNKNOWN
};
class HWP
{


};
#endif
