#ifndef __QW_PITA_FEEDBACK__
#define __QW_PITA_FEEDBACK__
/*
 * QwPitaFeedback.h
 *
 *  Created on: Sept 09, 2025
 *      Author: mrc
 *	Trial PITA feedback DataHandler. This class should be used to dynamically initalize
 *	the various PITA systems such as fPITAFB, fPITAPosU, and fPITAPosV
 *
 * Discusion:
 *	It isn't clear just yet if we will need individual DataHandler classes for each 
 *	Feedback system or if we could make a base 'Feedback' class from which we derive
 *	the other systems
 *
 *	All the feedback systems do seem to follow the same boilerplate:
 *		if(feedback_active==true)
 *			ApplyFeedback();                     -> ProcessData()
 *				GetChargeStat();                 	-> New GetStatistics();
 *					AccumulateRunningSum()       		-> Use fRunningSum
 *				Get EPICS DB Data                   -> GetEpics DB
 *				Apply Corrections
 *				Write back to EPICS with corrected values
 *	So, maybe we could just make a single Feedback class and we dynamically determine the
 *	runtime parameters through the a datahandler.map file? This seems likely.
 *
 *	Question: The feedback systems follows a similar structure but the specific implementations
 *	do not really...; for example, they all would need unique 'CalculateCorrection' function.
 *	Maybe I could group them by type?
 *		- All Hall[ABCD]IA  feedbacks are handled the same way
 *		- All PITA(POS[UV]) feedbacks are handled the same way
 *		- PosXYFB           feedback
 *	Total of 7 feedback systems, *3 unique ones*
 *	Conclusion: We will derive 3 unique data handlers for the 3 unique subystems
 *
 *	Question:
 *		How will we pass in the appropriate IOC to the appropriate Feedback system
 *		(thinking primarily for applying the corrections)?
 */

#include "VQWDataHandler.h"
#include "TString.h"

class QwPitaFeedback : public VQWDataHandler, public MQwDataHandlerCloneable<MyDataHandler>
{
public:
	QwPitaFeedback(const TString& name);
	QwPitaFeedback(const QwPitaFeedback &source);

	// Do we want to be deriving feedback systems from this class?
	// Or make unique classes for each system?
    virtual ~MyDataHandler();


};

#endif
