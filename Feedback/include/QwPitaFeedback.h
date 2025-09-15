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
 *
 *	If we use Alpha Pos U/V, and Delta Pos U/V (see RTP Cell Controls GUI) instead of modifying the
 *	HV values directly, maybe the correction equations will be general enough we can derive their
 *	functionality through mapfile cofiguration.
 */

#include "VQwDataHandler.h"
#include "QwEPICSControl.h"
#include "QwInsertableHalfWavePlate.h"
#include "QwFactory.h"

#include "TString.h"

#include <array>
#include <memory>

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
    Int_t ConnectChannels(QwSubsystemArrayParity& detectors) override;
    Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override;

	/*
    // Subsystems with support for subsystem arrays should override this
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

	/* Charge Asymmetry is based off of these BPMs
	I want to read these from a configuration file
	These BPMS are the target parameters (not used directly
	for PITA but are stored for posterity)
	std::vector<const char*> vBPM = {
		"bpm4aWS",
		"bpm4eWS",
		"bpm0i02aWS",
		"bpm1i06WS",
		"bpm12WS",
		"bpm0r05WS",
		"bpm0l06WS",
		"bpm0l05WS",
		"bpm0i05WS",
		"bpm2i02WS",
		"bpm2i01WS",
		"bcm0l02",
		"q_targ",
		"q_targC",
	};
	I need to be able to call
		MQwPublishable::RequestExternalValue(std::string,&fTargetParameter))
	where
	std::vector<target_parameters> vTargetParameters;
	class target_parameters {
	private:
		std::string mean_name;
		std::string width_name;
		double mean;
		double std;
		QwEPICS<double> mean_ioc;
		QwEPICS<double> std_ioc;
	public:
		target_parameters(string mean_ioc_name, string std_ioc_name)
		: mean_name(mean_ioc_name), std_name(std_ioc_name),
		  mean_ioc(mean_ioc_name), std_ioc(std_ioc_name), mean(0), std(0) { }
		set_mean();
		set_std();
		write();
	}
	then I could do
		for( auto const & param : target_parameters)
			RequestExternalValue(param.name, &fTargetParameter)
	If I make target_parameters a friend of QwPitaFeedback, then I can call
		fAsymmetry.RequestExternalValue(vBPM1[j],&fTargetParameter)
	directly inside the get and write functions
	or have a ReadValues(QwSubsystemArrayParity &fAsymmetry) and get the same
	effect


	 We need to be able to construct this object before we know any of the details. How to do that?
	 either need to offer a default constructor or we need to create it via a pointer.
	 The pointer approach is probably best else we would need to how the IHWP is instantiated
	 This issue can be sidestepped since I plan on making a vector<target_parameters>
	*/
private:
	class target_parameters
	{
	private:
		double mean;
		double width;
		std::string mean_ioc_name;
		std::string width_ioc_name;
		QwEPICS<double> mean_ioc;
		QwEPICS<double> width_ioc;
	public:
		target_parameters(std::string mean_name, std::string width_name)
		: mean(0), width(0),
		  mean_ioc_name(std::move(mean_name)), width_ioc_name(std::move(width_name)),
		  mean_ioc(mean_ioc_name)            , width_ioc(width_ioc_name) { }

		void SetMean(const double val)  { mean  = val; }
		void SetWidth(const double val) { width = val; }

		[[nodiscard]] double GetMean()  { return mean; }
		[[nodiscard]] double GetWidth() { return width; }

		/* I am not sure how this would actually work as a friend class
		   Might just want to have a 'GetIOCName instead'...
		if (fAsymmetry.RequestExternalValue(vBPM1[j],&fTargetParameter)){
		void Write(const VQwHardwareChannel* channel) {  return (channel.RequestExternalValue(vBPM1[j],&fTargetParameter)  ); }
		*/
	};

	friend target_parameters; // this scheme probably wont work
private:
	// What should the signiture look like?
	// Double_t fSlope_IN; // Read in from a .mapfile
	// Double_t fSlope_OUT;// Read in from a .mapfile

	/* There are two pita slope setpoint
		1) IHWP=IN
		2) IHWP=OUT
	   Need functionality of reading the IHWP status
	   Q: Why do we need fHalfWaveRevert boolean?

	   We need to be able to construct this object before we know any of the details. How to do that?
	   either need to offer a default constructor or we need to create it via a pointer.
	   The pointer approach is probably best else we would need to how the IHWP is instantiated
	*/
	class pita_slope
	{
	private:
		double slope_ihwp_in;
		double slope_ihwp_out;
		IHWP   ihwp;
		bool   revert_ihwp_status; // fHalfWaveRevert, why would we revert the status?
	public:
		pita_slope(const double slope_in, const double slope_out, bool revert = false)
		: slope_ihwp_in(slope_in), slope_ihwp_out(slope_out),
		  ihwp(), revert_ihwp_status(revert) { }
		pita_slope(std::string ioc_name, const double slope_in, const double slope_out, bool revert = false)
		: slope_ihwp_in(slope_in), slope_ihwp_out(slope_out),
		  ihwp(std::move(ioc_name)), revert_ihwp_status(revert) { }
	public:
		// Discussion:
		// IHWP can have status as IN, OUT, or UNKNOWN. Presumably,
		// the UNKNOWN state should not happen, but how to handle if it does?
		// Currently, we just assume the IHWP is set to OUT if it is in an
		// undeterminate state. Maybe we should return 0?
		[[nodiscard]]
		double GetSlope() noexcept {
			ihwp.Update();
			double slope = 0;
			if(ihwp == IHWP_STATUS::UNKNOWN) {
				QwWarning << "IHWP is in an indeterminate state: Setting PITA Slope to 0!" << QwLog::endl;
			} else {
				// XOR
				// ___| !Revert  |  Revert  |
				// OUT| SLOPE_OUT| SLOPE_IN |
				//  IN| SLOPE_IN | SLOPE_OUT|
				slope = ( ((ihwp==IHWP_STATUS::IN)!= revert_ihwp_status) ? slope_ihwp_in : slope_ihwp_out );
			}
			return slope;
		}
		IHWP_STATUS GetIHWPStatus() {
			return ihwp.GetIHWPStatus();
		}
		void dump() // TODO: remove me
		{
			QwMessage << "Printing class pita_slope info\n";
			QwMessage << "\tslope_ihwp_in    = " << slope_ihwp_in      << "\n";
			QwMessage << "\tslope_ihwp_out   = " << slope_ihwp_out     << "\n";
			QwMessage << "\trevert_ihwp_status = " << revert_ihwp_status << QwLog::endl;
		}
	};
	// DataHandler Arrays use the QwOptions::Define(Process)Options
	// but the individual datahandlers do not. This is an ugly method to
	// get configuration parameters into my datahandler and causes
	// me to delay the instantiation of internal objects -- mrc (09/15/25)
	struct ihwp_config_options
	{
		double slope_in, slope_out;
		bool revert_ihwp_state;
		std::string ihwp_ioc_channel;
		ihwp_config_options() : slope_in(0), slope_out(0), revert_ihwp_state(false), ihwp_ioc_channel("")
		{}
	};
	ihwp_config_options options;
	std::unique_ptr<pita_slope> slope;

};

#endif
