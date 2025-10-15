#ifndef __QWROOTFILEHANDLER_H__
#define __QWROOTFILEHANDLER_H__

#include <memory>
#include <array>
#include <string>
#include "QwOptions.h"
#include "QwRootFile.h"
#include "QwSubsystemArrayParity.h"

static constexpr unsigned N_ROOTFILE_TYPES = 3;
class QwRootFileHandler
{
public:
	enum class TYPE : int
	{
		TREE = 0,
		BURST,
		HISTO
	};

public:
	QwRootFileHandler();

public:
	/// \brief Sets internal flags based on the QwOptions
  	static void DefineOptions(QwOptions &options);
	void ProcessOptions(QwOptions &options);
public:
	void ConstructRootFiles(std::string label);
	void WriteParamFileList(std::string label, QwSubsystemArrayParity &detectors);

	template<typename T>
	void ConstructObjects(TYPE type, std::string label, T &system) {
		fRootFiles[static_cast<int>(type)]->ConstructObjects(label, system);
	}

	template<typename T>
	void ConstructHistograms(TYPE type, std::string label, T &system)
	{
		fRootFiles[static_cast<int>(type)]->ConstructHistograms(label, system);
	}

	template<typename T>
    void ConstructTreeBranches(TYPE type, std::string name, std::string desc, T &system, std::string prefix = "")
	{
#ifdef HAS_RNTUPLE_SUPPORT
		if(fEnableRNTuple) {
			fRootFiles[static_cast<int>(type)]->ConstructNTupleFields(name, desc, system, prefix);
		} else {
			fRootFiles[static_cast<int>(type)]->ConstructTreeBranches(name, desc, system, prefix);
		}
#else
		fRootFiles[static_cast<int>(type)]->ConstructTreeBranches(name, desc, system, prefix);
#endif
	}

	template<typename T>
	void FillTreeBranches(TYPE type, T &system)
	{
#ifdef HAS_RNTUPLE_SUPPORT
		if(fEnableRNTuple) {
			fRootFiles[static_cast<int>(type)]->FillNTupleFields(system);
		} else {
			fRootFiles[static_cast<int>(type)]->FillTreeBranches(system);
		}
#else
		fRootFiles[static_cast<int>(type)]->FillTreeBranches(system);
#endif
	}
	void FillTree(TYPE type, std::string label);

	template<typename T>
	void FillHistograms(TYPE type, T &system)
	{
		fRootFiles[static_cast<int>(type)]->FillHistograms(system);
	}

	void Finish();
	QwRootFile* GetFilePtr(TYPE type) const;
private:
	void Finish(TYPE type);
	void Write(TYPE type);
	void Close(TYPE type);
	
private:
	// Owning
    std::unique_ptr<QwRootFile>  fTreeRootFile;
    std::unique_ptr<QwRootFile>  fBurstRootFile;
    std::unique_ptr<QwRootFile>  fHistoRootFile;
	// Non-Owning
	std::array<QwRootFile*,  N_ROOTFILE_TYPES>  fRootFiles;
private:
	bool fSingleOutputFile;
#ifdef HAS_RNTUPLE_SUPPORT
	bool fEnableRNTuple;
#endif
};

static_assert( static_cast<QwRootFileHandler::TYPE>(N_ROOTFILE_TYPES-1) == QwRootFileHandler::TYPE::HISTO );
#endif
