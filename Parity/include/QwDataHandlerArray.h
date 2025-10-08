/**********************************************************\
* File: QwDataHandlerArray.h                           *
*                                                          *
* Author: P. M. King                                       *
* Time-stamp: <2009-02-04 10:30>                           *
\**********************************************************/

#ifndef __QWDATAHANDLERARRAY__
#define __QWDATAHANDLERARRAY__

#include <vector>
#include <map>
#include "Rtypes.h"
#include "TString.h"
#include "TDirectory.h"
#include <TTree.h>

// ROOT headers
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "QwDataHandlerArray.h"
#include "VQwDataHandler.h"
#include "QwOptions.h"
#include "QwHelicityPattern.h"
#include "MQwPublishable.h"

// Forward declarations
class QwParityDB;
class QwPromptSummary;

/**
 * \class QwDataHandlerArray
 * \ingroup QwAnalysis
 *
 * \brief Virtual base class for the parity handlers
 *
 *   Virtual base class for the classes containing the
 *   event-based information from each parity handler.
 *   This will define the interfaces used in communicating
 *   with the CODA routines.
 *
 */
template<typename Subsystem_t>
class QwDataHandlerArray:
    public std::vector<std::shared_ptr<VQwDataHandler<Subsystem_t>> >,
    public MQwPublishable<QwDataHandlerArray<Subsystem_t>,VQwDataHandler<Subsystem_t>>
{
 private:
  using HandlerPtrs = std::vector<std::shared_ptr< VQwDataHandler<Subsystem_t> > >;
 public:
  using const_iterator = typename HandlerPtrs::const_iterator;
  using iterator       = typename HandlerPtrs::iterator;
  using HandlerPtrs::begin;
  using HandlerPtrs::end;
  using HandlerPtrs::size;
  using HandlerPtrs::empty;

  private:
    /// Private default constructor
    QwDataHandlerArray(); // not implement, will thrown linker error on use

  public:
    /// Constructor from helicity pattern with options
    QwDataHandlerArray(QwOptions& options, Subsystem_t& subsystem, const TString &run);
    /// Constructor from subsystem array with options
    // QwDataHandlerArray(QwOptions& options, QwSubsystemArrayParity& detectors, const TString &run);
    /// Copy constructor by reference
    QwDataHandlerArray(const QwDataHandlerArray& source);
    /// Default destructor
    virtual ~QwDataHandlerArray();

    /// \brief Define configuration options for global array
    static void DefineOptions(QwOptions &options);
    /// \brief Process configuration options for the datahandler array itself
    void ProcessOptions(QwOptions &options);

    /// \brief Load from mapfile with T = helicity pattern or subsystem array
    void LoadDataHandlersFromParameterFile(QwParameterFile& mapfile, Subsystem_t& detectors, const TString &run);

    /// \brief Add the datahandler to this array
    void push_back(VQwDataHandler<Subsystem_t>* handler);
    void push_back(std::shared_ptr<VQwDataHandler<Subsystem_t>> handler);

    /// \brief Get the handler with the specified name
    VQwDataHandler<Subsystem_t>* GetDataHandlerByName(const TString& name);

    std::vector<VQwDataHandler<Subsystem_t>*> GetDataHandlerByType(const std::string& type);

    void ConstructTreeBranches(
        QwRootFile *treerootfile,
        const std::string& treeprefix = "",
        const std::string& branchprefix = "");

    /// \brief Construct a branch and vector for this handler with a prefix
    void ConstructBranchAndVector(TTree *tree, TString& prefix, std::vector <Double_t> &values);

    void FillTreeBranches(QwRootFile *treerootfile);
    /// \brief Fill the vector for this handler
    void FillTreeVector(std::vector<Double_t>& values) const;

    /// RNTuple methods
    void ConstructNTupleFields(
        QwRootFile *treerootfile,
        const std::string& treeprefix = "",
        const std::string& branchprefix = "");
    
    void FillNTupleFields(QwRootFile *treerootfile);

    /// Construct the histograms for this subsystem
    void  ConstructHistograms() {
      ConstructHistograms((TDirectory*) NULL);
    };
    /// Construct the histograms for this subsystem in a folder
    void  ConstructHistograms(TDirectory *folder) {
      TString prefix = "";
      ConstructHistograms(folder, prefix);
    };
    /// \brief Construct the histograms in a folder with a prefix
    void  ConstructHistograms(TDirectory *folder, TString &prefix);
    /// \brief Fill the histograms
    void  FillHistograms();

    /// \brief Fill the database
    void FillDB(QwParityDB *db, TString type);
    //    void FillErrDB(QwParityDB *db, TString type);
    //    const QwDataHandlerArray *dummy_source;

    void  ClearEventData();
    void  ProcessEvent();

    void UpdateBurstCounter(Short_t burstcounter)
	{
		if (!empty()) {
			for(iterator handler = begin(); handler != end(); ++handler){
				(*handler)->UpdateBurstCounter(burstcounter);
			}
		}
	}

    /// \brief Assignment operator
    QwDataHandlerArray& operator=  (const QwDataHandlerArray &value);
    /*
    /// \brief Addition-assignment operator
    QwDataHandlerArray& operator+= (const QwDataHandlerArray &value);
    /// \brief Subtraction-assignment operator
    QwDataHandlerArray& operator-= (const QwDataHandlerArray &value);
    /// \brief Sum of two handler arrays
    void Sum(const QwDataHandlerArray &value1, const QwDataHandlerArray &value2);
    /// \brief Difference of two handler arrays
    void Difference(const QwDataHandlerArray &value1, const QwDataHandlerArray &value2);
    /// \brief Scale this handler array
    void Scale(Double_t factor);
    */

    /// \brief Update the running sums for devices accumulated for the global error non-zero events/patterns
    void AccumulateRunningSum();

    /// \brief Update the running sums for devices accumulated for the global error non-zero events/patterns
    void AccumulateRunningSum(const QwDataHandlerArray& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
    /// \brief Update the running sums for devices check only the error flags at the channel level. Only used for stability checks
    void AccumulateAllRunningSum(const QwDataHandlerArray& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);

    /// \brief Calculate the average for all good events
    void CalculateRunningAverage();

    /// \brief Report the number of events failed due to HW and event cut failures
    void PrintErrorCounters() const;

    /// \brief Print value of all channels
    void PrintValue() const;

    
    void WritePromptSummary(QwPromptSummary *ps, TString type);
    
    
    void ProcessDataHandlerEntry();

    void FinishDataHandler();

  protected:

    void SetPointer(QwHelicityPattern& helicitypattern) {
      fHelicityPattern = &helicitypattern;
    }
    void SetPointer(QwSubsystemArrayParity& detectors) {
      fSubsystemArray = &detectors;
    }

    /// Pointer for the original data source
	Subsystem_t *fSubsystem;
    QwHelicityPattern *fHelicityPattern;
    QwSubsystemArrayParity *fSubsystemArray;

    /// Filename of the global detector map
    std::string fDataHandlersMapFile;

    Bool_t ScopeMismatch(TString name){
      name.ToLower();
      EDataHandlerArrayScope tmpscope = kUnknownScope;
      if (name=="event") tmpscope = kEventScope;
      if (name=="pattern") tmpscope = kPatternScope;
      return (fArrayScope != tmpscope);
    }
    enum EDataHandlerArrayScope {kUnknownScope=-1, kEventScope, kPatternScope};
    EDataHandlerArrayScope fArrayScope;

    std::vector<std::string> fDataHandlersDisabledByName; ///< List of disabled types
    std::vector<std::string> fDataHandlersDisabledByType; ///< List of disabled names

    Bool_t fPrintRunningSum;

    /// Test whether this handler array can contain a particular handler
    static Bool_t CanContain(VQwDataHandler<Subsystem_t>* handler) {
      return (dynamic_cast<VQwDataHandler<Subsystem_t>*>(handler) != 0);
    };

}; // class QwDataHandlerArray

#endif // __QWDATAHANDLERARRAY__
