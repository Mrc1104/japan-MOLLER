#pragma once

#include "QwRootTreeBranchVector.h"
#include "Rtypes.h"
#include "TTree.h"
#include "TDirectory.h"
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RNTupleWriter.hxx"
#include "ROOT/RField.hxx"
#include "ROOT/RFieldBase.hxx"
#include "ROOT/RField/RFieldRecord.hxx"

#include <string>
#include <memory>
#include <vector>

class VQwRootWriter
{
public:
   VQwRootWriter(std::shared_ptr<TDirectory> sink, std::string name, std::string desc, std::string prefix = "");
   virtual ~VQwRootWriter() = default;
public:
   bool IsInit();
   bool Initialize();
   bool Finalize();
   void AddField(QwRootTreeBranchVector &values);
   void AddFields(std::vector<QwRootTreeBranchVector*> values);
   std::size_t Fill();
private:
   virtual bool InitializeImpl() = 0;
   virtual bool FinalizeImpl()   = 0;
   virtual void AddFieldImpl(QwRootTreeBranchVector &values) = 0;
   virtual std::size_t FillImpl()= 0;
protected:
   // Writer Information
   std::shared_ptr<TDirectory> fFile;
   bool fWriterInitialized{false};
   bool fWriterFinalized{false};

protected:
   /// Generic Information: Name, description, Obj. Type
   const std::string fName;
   const std::string fDesc;
   const std::string fPrefix;
   std::string fType;
protected:
    std::vector<QwRootTreeBranchVector*> fBranches;
};
