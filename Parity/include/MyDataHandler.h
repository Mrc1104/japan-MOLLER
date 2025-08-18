#ifndef MYDATAHANDLER_H_H
#define MYDATAHANDLER_H_H
/*
 * MyDataHandler.h
 *
 *  Created on: Aug 08, 2025
 *      Author: mrc
 *  Test data handler creation
 *
 */

 /* Comments:
 		So it seems that the approach will be to make a "DataHandler" base class that inherits from VQWDataHandler and MQwDataHandlerCloneable<>
		VQWDataHandandler is our virtual interface class
		MQWDataHandlerCloneable<> is unknown

		We should set this DataHandler so that we can derive child classes from it

		For the task of updating QwFeedback, this will become our QwFeedback base class, from which we will derive feedback subsystems
		such as 
			* PITA,
			* Charge feedback,
			* etc
		ie. prminput/mock_datahandlers.map
		[QwFeedback]
			name       = PITA
			priority   = 10
			map        = PITA.conf

	TODO:
		* Q: What does MQwDataHandlerCloneable<> do?
		* A: It implements a "polymorphic" clone functionality through CRTP (so not really polymorphic). This allows us to call a unified
		     Clone method that (deep?) copies the subsystem rather than just copying the susbsytem pointer (shallow?).


		* Implement a mock data handler class

*/





// Parent Class
#include "VQwDataHandler.h"
#include "QwRootFile.h"
class QwRootFile;

class MyDataHandler :public VQwDataHandler, public MQwDataHandlerCloneable<MyDataHandler>
{
 public:
    typedef std::vector< VQwHardwareChannel* >::iterator       Iterator_HdwChan;
    typedef std::vector< VQwHardwareChannel* >::const_iterator ConstIterator_HdwChan;
 public:
    /// \brief Constructor with name
    MyDataHandler(const TString& name);

	/// \brief Copy constructor
	MyDataHandler(const MyDataHandler &source);

    /// Virtual destructor -- We will be inheriting from this class later on
    virtual ~MyDataHandler();

    /// \brief Load the channels and sensitivities
    Int_t LoadChannelMap(const std::string& mapfile) override;

    /// \brief Connect to Channels (event only)
	// Per Paul, this member varient will never be called
    Int_t ConnectChannels(QwSubsystemArrayParity& event) override;
    /// \brief Connect to Channels (asym & diff)
    Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override;

	// I Can just overload my own Create / Fill Tree functions
    void ConstructTreeBranches(QwRootFile *treerootfile,
        					   const std::string& treeprefix = "",
        					   const std::string& branchprefix = "") override;
    void FillTreeBranches(QwRootFile *treerootfile) override;
    void FillTreeVector(std::vector<Double_t> &values) const;

	// lets be explicit and say that we are overriding inherited functions
	void ParseConfigFile(QwParameterFile& file) override;
    void ProcessData() override;

	protected:
	// Default constructor (Protected for child class access)
	MyDataHandler() {};

    /// Error flag mask
    UInt_t fErrorFlagMask;
    const UInt_t* fErrorFlagPointer; // this "shadows" VQwDataHandler::fErrorFlagPtr
	                                 // Why? I don't see a good reason
									 // Both fErrorFlagPointer and VQwDataHandler::fErrorFlagPtr
									 // get set through calling QwSubsystemArrayParity::GetEventcutErrorFlagPointer().
									 // both are intialized by calling ConnectChannels. 
									 // Conclusion: They point to the same memory
	
	// List of channels to use in MyDataHandler
	// Declared in a mapfile as so:
	//   Parity/prminput/mock_datahandlers.map-3-[LRBCorrector]
	//   Parity/prminput/mock_datahandlers.map-4-  name       = LinRegBlue Corrector (run-level slopes)
	//   Parity/prminput/mock_datahandlers.map:5:  priority   = 10
	//   Parity/prminput/mock_datahandlers.map-6-  map        = mock_corrolator.conf
	//   Parity/prminput/mock_datahandlers.map-7-  slope-file-base = blueR
	// mock_corrolator.conf:
	//    # configuration for linRegblue ( must end with nonempty 'regvars/treetype'  block )
	//    #inpPath /home/cdaq/qweak/qwScratch/rootfiles/qwPass1_   : on cdaql5
	//    #inpPath  /home/MRVallee/japan/isu_sample_  :  pass3 production
	//    #customcut  jancut1 yield_bcm_ds.hw_sum>0
	//    #minEvents  1000
	//    #
	//    #.... place aliases before use as IVs or dVs
	//    #alias bpmdd9_9b=asym_qwk_bpm3h09WS-asym_qwk_bpm3h09bWS
	//    #alias asym_dd_Mdeven_odd=asym_qwk_mdevenbars-asym_qwk_mdoddbars
	//    # ...
	//    # dependent and Independent variables for Regression (arbitrary order)
	//    dv asym_tq15_r1      [Dependent Var] [type]_[name]
	//    iv diff_bpm_targetX  [Dependent Var] [type]_[name]
	// Note: the type can be [mps] too!

    /// List of channels to use in the combiner
    std::vector< const VQwHardwareChannel* > fIndependentVar;
    std::vector<  EQwHandleType> fIndependentType;
    std::vector<  std::string  > fIndependentName;
	std::vector<  std::string  > fIndependentFull;
	std::vector<  Double_t     > fIndependentValues;

    std::vector<  EQwHandleType> fDependentType;
    std::vector<  std::string  > fDependentName;
	std::vector<  std::string  > fDependentFull;
	/* fDependentValues is inherited VQwDataHandler */

	// fOuputVar and fOuputValues are inherited from VQwDataHandler
	// std::vector< VQwHardwareChannel* > fOutputVar;
	// std::vector< Double_t > fOutputValues;
	std::vector< VQwHardwareChannel* > fOutputVar_other;
	std::vector< Double_t > fOutputValues_other;

};




#endif
