#include "QwRootFileHandler.h"

QwRootFileHandler::QwRootFileHandler()
: fTreeRootFile{}
, fBurstRootFile{}
, fHistoRootFile{}
, fSingleOutputFile{false}
#ifdef HAS_RNTUPLE_SUPPORT
, fEnableRNTuple{false}
#endif
{ }

void QwRootFileHandler::DefineOptions(QwOptions &options)
{
  gQwOptions.AddOptions()("single-output-file", po::value<bool>()->default_bool_value(false), "Write a single output file");
#ifdef HAS_RNTUPLE_SUPPORT
  gQwOptions.AddOptions()("enable_rntuples", po::value<bool>()->default_bool_value(false), "Enable RNTuple Output");
#endif
}

void QwRootFileHandler::ProcessOptions(QwOptions &options)
{
	fSingleOutputFile = options.GetValue<bool>("single-output-file");
#ifdef HAS_RNTUPLE_SUPPORT
	fEnableRNTuple    = options.GetValue<bool>("enable-rntuples");
#endif
}

void QwRootFileHandler::ConstructRootFiles(std::string label)
{
	if(fSingleOutputFile) {
		fTreeRootFile = std::make_unique<QwRootFile>(label);
	} else {
		fTreeRootFile = std::make_unique<QwRootFile>(label+".trees");
		fBurstRootFile= std::make_unique<QwRootFile>(label+".bursts");
		fHistoRootFile= std::make_unique<QwRootFile>(label+".histos");
	}
	fRootFiles[static_cast<int>(TYPE::TREE)]  = (fTreeRootFile.get());
	fRootFiles[static_cast<int>(TYPE::BURST)] = (fBurstRootFile) ? fBurstRootFile.get() : fTreeRootFile.get();
	fRootFiles[static_cast<int>(TYPE::HISTO)] = (fHistoRootFile) ? fHistoRootFile.get() : fTreeRootFile.get();
}

void QwRootFileHandler::WriteParamFileList(std::string label, QwSubsystemArrayParity &detectors)
{
	if(fSingleOutputFile) {
		// Save everything in the TYPE::TREE file
		fRootFiles[static_cast<int>(TYPE::TREE)]->WriteParamFileList("mapfiles", detectors);
	} else {
		fRootFiles[static_cast<int>(TYPE::TREE)]->WriteParamFileList("mapfiles", detectors);
		fRootFiles[static_cast<int>(TYPE::BURST)]->WriteParamFileList("mapfiles", detectors);
		fRootFiles[static_cast<int>(TYPE::HISTO)]->WriteParamFileList("mapfiles", detectors);
	}
}

void QwRootFileHandler::FillTree(TYPE type, std::string label)
{
#ifdef HAS_RNTUPLE_SUPPORT
	if(fEnableRNTuple) {
		fRootFiles[static_cast<int>(type)]->FillNTuple(label);
	} else {
		fRootFiles[static_cast<int>(type)]->FillTree(label);
	}
#else
	fRootFiles[static_cast<int>(type)]->FillTree(label);
#endif
}

void QwRootFileHandler::Finish()
{
#ifdef HAS_RNTUPLE_SUPPORT
	if(fSingleOutputFile) {
		if(fEnableRNTuple) {
        	// RNTuple-only mode: use Close() for proper RNTuple finalization
			Close(TYPE::TREE);
		} else {
        	Finish(TYPE::TREE);
		}
	} else { // !fSingleOutputFile
		if(fEnableRNTuple) {
        	// RNTuple-only mode: use Close() for proper RNTuple finalization
			Close(TYPE::TREE);
			Close(TYPE::HISTO);
			Close(TYPE::BURST);
		} else {
        	Finish(TYPE::TREE);
        	Finish(TYPE::HISTO);
        	Finish(TYPE::BURST);
		}
	}
#else
	if(fSingleOutputFile) {
        Finish(TYPE::TREE);
	} else {
        Finish(TYPE::TREE);
        Finish(TYPE::HISTO);
        Finish(TYPE::BURST);
	}
#endif
}
void QwRootFileHandler::Finish(TYPE type)
{
	Write(type);
	Close(type);
}

void QwRootFileHandler::Write(TYPE type)
{
 	/* We need to delete any old cycles (TObject::kOverwrite)  */
	fRootFiles[static_cast<int>(type)]->Write(0, TObject::kOverwrite);
}

void QwRootFileHandler::Close(TYPE type)
{
	fRootFiles[static_cast<int>(type)]->Close();
}

QwRootFile* QwRootFileHandler::GetFilePtr(TYPE type) const
{
	return fRootFiles[static_cast<int>(type)];
}
