#ifndef __QWFEEDBACKHANDLER__
#define __QWFEEDBACKHANDLER__
/*
 * QwFeedbackHandler.h
 *
 *  Created on: Aug 20, 2025
 *      Author: mrc
 *  Historically, the Feedback logic was implemented in one large 'QwHelicityCorrelatedFeedback'
 *	class that had the logic hardcoded in. The goal of this class is to decouple the different
 *	feedback systems using the pre-existing VQwDataHandler interface.
 *	
 *	Current Questions:
 *		Q1: How will we handler feedback-specific implementations? PITA slopes are different than
 *		    charge feedback. Will the simply be derived from this class?
 *		Q1.1: How does QwDataHandlerArray mul determine the fsensitivities?
 *      A1.1: The fsenstivites are written into nxm array where each entry corresponds to a 
 *			  dependent_var(n_i) x independent_var(m_j). Essentially, we know the order in which
 *			  we write them to file
 *		Q2: What are the feedback systems we will need to implement
 *		A2: PITA-Feedback, HC-IA-Feedback, HA-IA-Feedback, HB-IA-Feedback, PITA-POSU-Feedback,
 *          PITA-POSV-Feedback, POSXY-Feedback.
 *		Q2.1: How do we calculate the PITA Feedback?
 *		Q2.2: How do we calculate the   IA Feedback?
 *		Q2.3: How do we calculate the   IA Feedback?
 *		Q2.3: How do we calculate the POSXYFeedback?
 *
 *
 *
 *	Task List:
 *		1. Implement PITA-Feedback-Generic
 *		1.1 Can we derive specific PITA-*-Feedback functionality via mapfile configuration?
 *		2. Implement IA-Feedback-Generic
 *		2.1 Can we derive specific H*-IA-Feedback functionality via mapfile configuration?
 *		3. Implement POXY-Feedback
 *		
 */



#endif
