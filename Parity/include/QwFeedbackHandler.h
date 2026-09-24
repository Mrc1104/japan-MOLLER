#pragma once
#include "VQwDataHandler.h"
#include "QwParameterFile.h"
#include "ErrorHandling.h"
#include <QwFeedback.h>
#include <source_location>
#include <string>
#include <ostream>


class QwFeedbackHandler : public VQwDataHandler, public MQwDataHandlerCloneable<QwFeedbackHandler>
{
public:
	QwFeedbackHandler(TString const& name);
	QwFeedbackHandler(QwFeedbackHandler const& source);
    ~QwFeedbackHandler() {}
public:
	// All inherited functions
	void ParseConfigFile(QwParameterFile& file) override;
	Int_t ConnectChannels(QwSubsystemArrayParity& yield, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override;
    // Subsystems with support for subsystem arrays should override this
    Int_t ConnectChannels(QwSubsystemArrayParity& /*detectors*/) override { THROW_ERROR("NOT SUPPORTED"); }
    void ProcessData() override;
    void UpdateBurstCounter(Short_t burstcounter) override { THROW_ERROR("NOT SUPPORTED"); }
    void FinishDataHandler() override;
    void ClearEventData() override;
    void AccumulateRunningSum(VQwDataHandler &value, Int_t count = 0, Int_t ErrorMask = 0xFFFFFFF) override { THROW_ERROR("NOT SUPPORTED"); }

    void ConstructTreeBranches( QwRootFile *treerootfile, const std::string& treeprefix = "", const std::string& branchprefix = "") override;
    void FillTreeBranches(QwRootFile *treerootfile) override;
    void ConstructNTupleFields( QwRootFile *treerootfile, const std::string& treeprefix = "", const std::string& branchprefix = "") override;
    void FillNTupleFields(QwRootFile *treerootfile) override;

    /// \brief Construct the histograms in a folder with a prefix
    void  ConstructHistograms(TDirectory * /*folder*/, TString & /*prefix*/) override;
    /// \brief Fill the histograms
    void  FillHistograms() override;

protected:
    Int_t LoadChannelMap(const std::string&) override;
    Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override { THROW_ERROR("NOT SUPPORTED"); }

private:
	std::size_t fMaxPattern;
	std::size_t fPatternCounter;
   	VQwHardwareChannel const* fDeviceObserver;
   	VQwHardwareChannel* fDeviceAccum;
	std::unique_ptr<QwFeedback> fFeedback;
};

// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(QwFeedbackHandler);
