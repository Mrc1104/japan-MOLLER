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
 *	Possible Soln (09/08/25): We could 'hardcode' in the correction functions for the different
 *	PITA(POS[UV]) feedbacks and then use a std::function<> as a func pointer to the correct
 *	correction routine to call. 
 *		Pros: No need to worry about the different setpoints in the mapfile (just define them all
 *		and a PITA-TYPE (=0,1,2) could be set to select the appropriate one.
 *		Pros: No need to worry about how to dynamically select the correct correction function (this 
 *		is the problem we are solving)
 *		Cons: This is solving the 'hardcoding' problem with more hardcoding
 *		Cons: Makes our FeedBack.map configuration very rigid and requires several new options
 *		Cons: We would need to specify what pubished var we are accumulating ('x_targ' or 'y_targ')
 */

#include "VQwDataHandler.h"
#include "QwEPICSControl.h"
#include "QwFactory.h"

#include "TString.h"

#include <array>

class QwPitaFeedback : public VQwDataHandler,
                             public MQwDataHandlerCloneable<QwPitaFeedback>
{
public:
	QwPitaFeedback(const TString& name);
	QwPitaFeedback(const QwPitaFeedback &source);

	// Do we want to be deriving feedback systems from this class?
	// Or make unique classes for each system?
    virtual ~QwPitaFeedback();
	
public: // Inherited Functions

    /// \brief Load the channels and sensitivities
    Int_t LoadChannelMap(const std::string& mapfile) override;
    void ParseConfigFile(QwParameterFile& file) override;
	/*
    Int_t ConnectChannels(QwSubsystemArrayParity& yield, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff){
    virtual Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff);
    // Subsystems with support for subsystem arrays should override this
    virtual Int_t ConnectChannels(QwSubsystemArrayParity& detectors) { return 0; }
    virtual void ProcessData();
    virtual void UpdateBurstCounter(Short_t burstcounter){fBurstCounter=burstcounter;};
    virtual void FinishDataHandler(){
    virtual void ClearEventData();
    virtual void AccumulateRunningSum(VQwDataHandler &value, Int_t count = 0, Int_t ErrorMask = 0xFFFFFFF);
    virtual void ConstructTreeBranches(
    virtual void FillTreeBranches(QwRootFile *treerootfile);
    /// \brief Construct the histograms in a folder with a prefix
    virtual void  ConstructHistograms(TDirectory *folder, TString &prefix) { };
    /// \brief Fill the histograms
    virtual void  FillHistograms() { };
    /// \brief Publish all variables of the subsystem
    virtual Bool_t PublishInternalValues() const;
    /// \brief Try to publish an internal variable matching the submitted name
    virtual Bool_t PublishByRequest(TString device_name);
	*/

private:
	// QwEPICS<double> // I need to delay the construction of this object,
					   // because we do not know the IOC name until after we
					   // we parse the configuration file!
					   // One Soln: Smart pointers and dynamically allocate them
	// For now, we will hardcode them and isntantiate inside the constructor
	// Can we get a more discriptive naming scheme?
	QwEPICS<Double_t> setpoint1;
	QwEPICS<Double_t> setpoint2;
	QwEPICS<Double_t> setpoint3;
	QwEPICS<Double_t> setpoint4;
	QwEPICS<Double_t> setpoint5;
	QwEPICS<Double_t> setpoint6;
	QwEPICS<Double_t> setpoint7;
	QwEPICS<Double_t> setpoint8;

	Double_t fSlope_IN; // Read in from a .mapfile
	Double_t fSlope_OUT;// Read in from a .mapfile

};

#endif
