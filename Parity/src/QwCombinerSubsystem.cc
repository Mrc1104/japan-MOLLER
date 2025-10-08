/*
 * QwCombinerSubsystem.cc
 *
 *  Created on: Aug 11, 2011
 *      Author: meeker
 */

#include "QwCombinerSubsystem.h"
#include "VQwSubsystemParity.h"
#include "QwCombiner.h"
#include "QwSubsystemArrayParity.h"
#include "QwParameterFile.h"

RegisterSubsystemFactory(QwCombinerSubsystem<QwSubsystemArrayParity>);


template<typename Type>  QwCombinerSubsystem<Type>::~QwCombinerSubsystem()
{
}

struct null_deleter { 
  void operator()(void const *) const { }
};

template<typename Type> std::shared_ptr<VQwSubsystem>  QwCombinerSubsystem<Type>::GetSharedPointerToStaticObject(){
  std::shared_ptr<VQwSubsystem> px(this, null_deleter());
  return px;
}

template<typename Type> VQwSubsystem&  QwCombinerSubsystem<Type>::operator=(VQwSubsystem* value)
{
  QwCombinerSubsystem* input= dynamic_cast<QwCombinerSubsystem*>(value);
  if (input!=NULL) {
    for(size_t i = 0; i < input->fDependentVar.size(); i++) {
      this->fOutputVar.at(i)->AssignValueFrom(input->fOutputVar.at(i));
    }
  }
  return *this;
}

template<typename Type> VQwSubsystem&  QwCombinerSubsystem<Type>::operator+=(VQwSubsystem* value)
{
  QwCombinerSubsystem* input = dynamic_cast<QwCombinerSubsystem*>(value);
  if (input!=NULL) {
    for(size_t i = 0; i < input->fDependentVar.size(); i++) {
      this->fOutputVar.at(i)->AddValueFrom(input->fOutputVar.at(i));
    }
  }
  return *this;
}

template<typename Type> VQwSubsystem&  QwCombinerSubsystem<Type>:: operator-=(VQwSubsystem* value)
{
  QwCombinerSubsystem* input = dynamic_cast<QwCombinerSubsystem*>(value);
  if (input!=NULL) {
    for(size_t i = 0; i<input->fDependentVar.size(); i++) {
      this->fOutputVar.at(i)->SubtractValueFrom(input->fOutputVar.at(i));
    }
  }
  return *this;
}

template<typename Type> VQwSubsystem&  QwCombinerSubsystem<Type>:: operator*=(VQwSubsystem* value)
{
  QwCombinerSubsystem* input = dynamic_cast<QwCombinerSubsystem*>(value);
  if (input!=NULL) {
    for(size_t i = 0; i<input->fDependentVar.size(); i++) {
      this->fOutputVar.at(i)->MultiplyBy(input->fOutputVar.at(i));
    }
  }
  return *this;
}

template<typename Type> VQwSubsystem&  QwCombinerSubsystem<Type>:: operator/=(VQwSubsystem* value)
{
  QwCombinerSubsystem* input = dynamic_cast<QwCombinerSubsystem*>(value);
  if (input!=NULL) {
    for(size_t i = 0; i<input->fDependentVar.size(); i++) {
      this->fOutputVar.at(i)->DivideBy(input->fOutputVar.at(i));
    }
  }
  return *this;
}


template<typename Type> void  QwCombinerSubsystem<Type>::Ratio(VQwSubsystem* value1, VQwSubsystem* value2)
{
  *this = value1;
  *this /= value2;
}


template<typename Type> void  QwCombinerSubsystem<Type>::Scale(Double_t value)
{ 
  for(size_t i = 0; i < this->fDependentVar.size(); i++)
  {
    this->fOutputVar.at(i)->Scale(value);
  }
  
};

template<typename Type> void  QwCombinerSubsystem<Type>::AccumulateRunningSum(VQwSubsystem* input, Int_t count, Int_t ErrorMask)
{
  QwCombinerSubsystem* value = dynamic_cast<QwCombinerSubsystem*> (input);
  if (value!=NULL){
    QwCombiner<Type>::AccumulateRunningSum(*value, count, ErrorMask);
  }
}

template<typename Type> void  QwCombinerSubsystem<Type>::DeaccumulateRunningSum(VQwSubsystem* input, Int_t ErrorMask)
{
  QwCombinerSubsystem* value = dynamic_cast<QwCombinerSubsystem*> (input);
  if (value!=NULL) {
    for (size_t i = 0; i < value-> fDependentVar.size(); i++) {
      fOutputVar.at(i)->DeaccumulateRunningSum(value->fOutputVar.at(i), ErrorMask);
    }
  }
}

template<typename Type> void  QwCombinerSubsystem<Type>::CalculateRunningAverage()
{
  QwCombiner<Type>::CalculateRunningAverage();
}

template<typename Type> void  QwCombinerSubsystem<Type>:: PrintValue() const{
  QwCombiner<Type>::PrintValue();
}


template<typename Type> void  QwCombinerSubsystem<Type>::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  for (size_t i = 0; i < fDependentVar.size(); i++){
    fOutputVar.at(i)->ConstructHistograms(folder,prefix);
  }
};

template<typename Type> void  QwCombinerSubsystem<Type>::FillHistograms()
{
  for (size_t i = 0; i < fDependentVar.size(); i++){
    fOutputVar.at(i)->FillHistograms();
  }
};

template<typename Type> void  QwCombinerSubsystem<Type>::DeleteHistograms()
{
  for (size_t i = 0; i < fDependentVar.size(); i++){
    //    fOutputVar.at(i)->DeleteHistograms();
  }
};

template<typename Type> void  QwCombinerSubsystem<Type>::ConstructBranch(TTree *tree, TString & prefix)
{
  for (size_t i = 0; i < fDependentVar.size(); i++){
    fOutputVar.at(i)->ConstructBranch(tree, prefix);
  }
};

template<typename Type> void  QwCombinerSubsystem<Type>::ConstructBranch(TTree *tree, TString & prefix, QwParameterFile& trim_file)
{
  TString tmp;
  QwParameterFile* nextmodule;
  trim_file.RewindToFileStart();
  tmp="Combiner";
  trim_file.RewindToFileStart();
  if (trim_file.FileHasModuleHeader(tmp)){
    nextmodule=trim_file.ReadUntilNextModule();//This section contains sub modules and or channels to be included in the tree
    for (size_t i = 0; i < fDependentVar.size(); i++){
      fOutputVar.at(i)->ConstructBranch(tree, prefix, *nextmodule);
    }
  }
};


template<typename Type> void  QwCombinerSubsystem<Type>::IncrementErrorCounters()
{
  /// TODO:  Write QwCombinerSubsystem::IncrementErrorCounters
}

template<typename Type> void  QwCombinerSubsystem<Type>::UpdateErrorFlag(const VQwSubsystem *ev_error){
  /// TODO:  Write QwCombinerSubsystem::UpdateErrorFlag
  //if (Compare(ev_error)){
  //QwCombinerSubsystem* input = dynamic_cast<QwCombinerSubsystem*> (ev_error);
  //}  
};


/// DERIVED FUNCTIONS /// 


/*  All of the functions below are using generic
 * returns for testing purposes. 
 */



template<typename Type> Int_t  QwCombinerSubsystem<Type>::LoadChannelMap(TString)
{
  Int_t sample = 0;
  return sample;
}


template<typename Type> Int_t  QwCombinerSubsystem<Type>::LoadInputParameters(TString)
{
  Int_t sample = 0;
  return sample;
}


template<typename Type> Int_t  QwCombinerSubsystem<Type>::LoadEventCuts(TString)
{
  Int_t sample = 0;
  return sample;
  
}

template<typename Type> Int_t  QwCombinerSubsystem<Type>::ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words)
{
  Int_t sample = 0;
  return sample;
}


template<typename Type> Int_t  QwCombinerSubsystem<Type>::ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words)
{
  Int_t sample = 0;
  return sample;
}


template<typename Type> Bool_t  QwCombinerSubsystem<Type>::ApplySingleEventCuts()
{
  return true;
}

template<typename Type> void  QwCombinerSubsystem<Type>::PrintErrorCounters() const
{
}

template<typename Type> UInt_t  QwCombinerSubsystem<Type>::GetEventcutErrorFlag()
{
    return 0;
  
}

template class QwCombinerSubsystem<QwSubsystemArrayParity>;
