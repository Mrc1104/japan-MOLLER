#pragma once
#include "VQwDataHandler.h"
#include <source_location>
#include <string>


[[noreturn]] inline void throw_with_location(
    const std::string& message,
    const std::source_location location = std::source_location::current())
{
    std::string full_msg = std::string(location.file_name()) + ":" 
                         + std::to_string(location.line()) + " in " 
                         + location.function_name() + " -> " + message;
    throw std::runtime_error(full_msg);
}
#define THROW_ERROR(msg) throw_with_location(msg)

class QwFeedbackHandler : public VQwDataHandler
{
public:
	QwFeedbackHandler(TString const& name);
	QwFeedbackHandler(QwFeedbackHandler const& source);
    ~QwFeedbackHandler() {}
public:
	// All inherited functions
	void ParseConfigFile(QwParameterFile& file) override;
	Int_t ConnectChannels(QwSubsystemArrayParity& /*yield*/, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override { THROW_ERROR("NOT SUPPORTED"); }
    // Subsystems with support for subsystem arrays should override this
    Int_t ConnectChannels(QwSubsystemArrayParity& /*detectors*/) override { THROW_ERROR("NOT SUPPORTED"); }
    void ProcessData() override { THROW_ERROR("NOT SUPPORTED"); }
    void UpdateBurstCounter(Short_t burstcounter) override { THROW_ERROR("NOT SUPPORTED"); }
    void FinishDataHandler() override { THROW_ERROR("NOT SUPPORTED"); }
    void ClearEventData() override { THROW_ERROR("NOT SUPPORTED"); }
    void AccumulateRunningSum(VQwDataHandler &value, Int_t count = 0, Int_t ErrorMask = 0xFFFFFFF) override { THROW_ERROR("NOT SUPPORTED"); }

    void ConstructTreeBranches( QwRootFile *treerootfile, const std::string& treeprefix = "", const std::string& branchprefix = "") override { THROW_ERROR("NOT SUPPORTED"); }
    void FillTreeBranches(QwRootFile *treerootfile) override { THROW_ERROR("NOT SUPPORTED"); }
    void ConstructNTupleFields( QwRootFile *treerootfile, const std::string& treeprefix = "", const std::string& branchprefix = "") { THROW_ERROR("NOT SUPPORTED"); }
    void FillNTupleFields(QwRootFile *treerootfile) override { THROW_ERROR("NOT SUPPORTED"); }

    /// \brief Construct the histograms in a folder with a prefix
    void  ConstructHistograms(TDirectory * /*folder*/, TString & /*prefix*/) override { THROW_ERROR("NOT SUPPORTED"); }
    /// \brief Fill the histograms
    void  FillHistograms() override { THROW_ERROR("NOT SUPPORTED"); }

protected:
    Int_t LoadChannelMap(const std::string&) override;
    Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override { THROW_ERROR("NOT SUPPORTED"); }
};

// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(QwFeedbackHandler);
