/**********************************************************\
* File: QwDataHandlerArray.cc                         *
*                                                         *
* Author:                                                 *
* Time-stamp:                                             *
\**********************************************************/

#include "QwDataHandlerArray.h"

// System headers
#include <stdexcept>

// Qweak headers
#include "VQwDataHandler.h"
#include "QwParameterFile.h"
#include "QwHelicityPattern.h"

//*****************************************************************//
/**
 * Create a handler array based on the configuration option 'detectors'
 */
template<typename Handler> QwDataHandlerArray<Handler>::QwDataHandlerArray(QwOptions& options, QwHelicityPattern& helicitypattern, const TString &run)
  : fHelicityPattern(0),fSubsystemArray(0),fDataHandlersMapFile(""),fArrayScope(kPatternScope)
{
  ProcessOptions(options);
  if (fDataHandlersMapFile != ""){
    QwParameterFile mapfile(fDataHandlersMapFile.c_str());
    QwMessage << "Loading handlers from " << fDataHandlersMapFile << "." << QwLog::endl;
    LoadDataHandlersFromParameterFile(mapfile, helicitypattern, run);
  }
}

/**
 * Create a handler array based on the configuration option 'detectors'
 */
template<typename Handler> QwDataHandlerArray<Handler>::QwDataHandlerArray(QwOptions& options, QwSubsystemArrayParity& detectors, const TString &run)
  : fHelicityPattern(0),fSubsystemArray(0),fDataHandlersMapFile(""),fArrayScope(kEventScope)
{
  ProcessOptions(options);
  if (fDataHandlersMapFile != ""){
    QwParameterFile mapfile(fDataHandlersMapFile.c_str());
    QwMessage << "Loading handlers from " << fDataHandlersMapFile << "." << QwLog::endl;
    LoadDataHandlersFromParameterFile(mapfile, detectors, run);
  }
}

/**
 * Copy constructor by reference
 * @param source Source handler array
 */
template<typename Handler> QwDataHandlerArray<Handler>::QwDataHandlerArray(const QwDataHandlerArray& source)
: fHelicityPattern(source.fHelicityPattern),
  fSubsystemArray(source.fSubsystemArray),
  fDataHandlersMapFile(source.fDataHandlersMapFile),
  fDataHandlersDisabledByName(source.fDataHandlersDisabledByName),
  fDataHandlersDisabledByType(source.fDataHandlersDisabledByType)
{
  // Make copies of all handlers rather than copying just the pointers
  for (const_iterator handler = source.begin(); handler != source.end(); ++handler) {
    this->push_back(handler->get()->Clone());
    /*
    // Instruct the handler to publish variables
    if (this->back()->PublishInternalValues() == kFALSE) {
      QwError << "Not all variables for " << this->back()->GetName()
             << " could be published!" << QwLog::endl;
    */
  }
}



//*****************************************************************//

/// Destructor
template<typename Handler> QwDataHandlerArray<Handler>::~QwDataHandlerArray()
{
  // nothing
}

/**
 * Fill the handler array with the contents of a map file
 * @param detectors Map file
 */
template<typename Handler> void QwDataHandlerArray<Handler>::LoadDataHandlersFromParameterFile(
    QwParameterFile& mapfile,
    T& detectors,
    const TString &run)
{
  // Set pointer to this object
  SetPointer(detectors);

  // This is how this should work
  QwParameterFile* preamble;
  preamble = mapfile.ReadSectionPreamble();
  // Process preamble
  QwVerbose << "Preamble:" << QwLog::endl;
  QwVerbose << *preamble << QwLog::endl;
  if (preamble) delete preamble;

  QwParameterFile* section;
  std::string section_name;
  while ((section = mapfile.ReadNextSection(section_name))) {

    // Debugging output of configuration section
    QwVerbose << "[" << section_name << "]" << QwLog::endl;
    QwVerbose << *section << QwLog::endl;

    // Determine type and name of handler
    std::string handler_type = section_name;
    std::string handler_name;
    std::string handler_scope;
    if (! section->FileHasVariablePair("=","name",handler_name)) {
      QwError << "No name defined in section for handler " << handler_type << "." << QwLog::endl;
      delete section; section = 0;
      continue;
    }
    if (section->FileHasVariablePair("=","scope",handler_scope)) {
      if (ScopeMismatch(handler_scope)) continue;
    } else {
      //  Assume the scope of a handler without a scope specifier is
      //  "pattern".
      if (fArrayScope != kPatternScope) continue;
    }

    // If handler type is explicitly disabled
    bool disabled_by_type = false;
    for (size_t i = 0; i < fDataHandlersDisabledByType.size(); i++)
      if (handler_type == fDataHandlersDisabledByType.at(i))
        disabled_by_type = true;
    if (disabled_by_type) {
      QwWarning << "DataHandler of type " << handler_type << " disabled." << QwLog::endl;
      delete section; section = 0;
      continue;
    }

    // If handler name is explicitly disabled
    bool disabled_by_name = false;
    for (size_t i = 0; i < fDataHandlersDisabledByName.size(); i++)
      if (handler_name == fDataHandlersDisabledByName.at(i))
        disabled_by_name = true;
    if (disabled_by_name) {
      QwWarning << "DataHandler with name " << handler_name << " disabled." << QwLog::endl;
      delete section; section = 0;
      continue;
    }

    // Create handler
    QwMessage << "Creating handler of type " << handler_type << " "
              << "with name " << handler_name << "." << QwLog::endl;
    VQwDataHandler<Handler>* handler = 0;
    
    try {
      handler =
        VQwDataHandlerFactory<Handler>::Create(handler_type, handler_name);
    } catch (QwException_TypeUnknown&) {
      QwError << "No support for handlers of type " << handler_type << "." << QwLog::endl;
      // Fall-through to next error for more the psychological effect of many warnings
    }
    if (! handler) {
      QwError << "Could not create handler " << handler_type << "." << QwLog::endl;
      delete section; section = 0;
      continue;
    }

    // If this handler cannot be stored in this array
    if (! CanContain(handler)) {
      QwMessage << "DataHandler " << handler_name << " cannot be stored in this "
                << "handler array." << QwLog::endl;
      QwMessage << "Deleting handler " << handler_name << " again" << QwLog::endl;
      delete section; section = 0;
      delete handler; handler = 0;
      continue;
    }

    // Pass detector maps
    handler->SetParent(this);
    handler->SetRunLabel(run);
    handler->SetPointer(&detectors);
    handler->ParseConfigFile(*section);
    handler->LoadChannelMap();
    handler->ConnectChannels(detectors);
    handler->InitRunningSum();

    // Add to array
    this->push_back(handler);
    /*    
    // Instruct the handler to publish variables
    if (handler->PublishInternalValues() == kFALSE) {
      QwError << "Not all variables for " << handler->GetName()
              << " could be published!" << QwLog::endl;
    }
    */
    // Delete parameter file section
    delete section; section = 0;
  }
}
//*****************************************************************

/**
 * Add the handler to this array.  Do nothing if the handler is null or if
 * there is already a handler with that name in the array.
 * @param handler DataHandler to add to the array
 */
template<typename Handler> void QwDataHandlerArray<Handler>::push_back(VQwDataHandler<Handler>* handler)
{
  if (handler == NULL) {
    QwError << "QwDataHandlerArray::push_back(): NULL handler"
            << QwLog::endl;
    //  This is an empty handler...
    //  Do nothing for now.

  } else if (!this->empty() && GetDataHandlerByName(handler->GetName())){
    //  There is already a handler with this name!
    QwError << "QwDataHandlerArray::push_back(): handler " << handler->GetName()
            << " already exists" << QwLog::endl;

  } else if (!CanContain(handler)) {
    //  There is no support for this type of handler
    QwError << "QwDataHandlerArray::push_back(): handler " << handler->GetName()
            << " is not supported by this handler array" << QwLog::endl;

  } else {
    std::shared_ptr<VQwDataHandler<Handler>> handler_tmp(handler);
    HandlerPtrs::push_back(handler_tmp);

    // Set the parent of the handler to this array
    //    handler_tmp->SetParent(this);

  }
}

/**
 * Define configuration options for global array
 * @param options Options
 */
template<typename Handler> void QwDataHandlerArray<Handler>::DefineOptions(QwOptions &options)
{
  options.AddOptions()("datahandlers",
                       po::value<std::string>(),
                       "map file with datahandlers to include");

  // Versions of boost::program_options below 1.39.0 have a bug in multitoken processing
#if BOOST_VERSION < 103900
  options.AddOptions()("DataHandler.disable-by-type",
                       po::value<std::vector <std::string> >(),
                       "handler types to disable");
  options.AddOptions()("DataHandler.disable-by-name",
                       po::value<std::vector <std::string> >(),
                       "handler names to disable");
#else // BOOST_VERSION >= 103900
  options.AddOptions()("DataHandler.disable-by-type",
                       po::value<std::vector <std::string> >()->multitoken(),
                       "handler types to disable");
  options.AddOptions()("DataHandler.disable-by-name",
                       po::value<std::vector <std::string> >()->multitoken(),
                       "handler names to disable");
#endif // BOOST_VERSION
}


/**
 * Handle configuration options for the handler array itself
 * @param options Options
 */
template<typename Handler> void QwDataHandlerArray<Handler>::ProcessOptions(QwOptions &options)
{
  // Filename to use for handler creation (single filename could be expanded
  // to a list)
  if (options.HasValue("datahandlers")){
    fDataHandlersMapFile = options.GetValue<std::string>("datahandlers");
  }
  // DataHandlers to disable
  fDataHandlersDisabledByName = options.GetValueVector<std::string>("DataHandler.disable-by-name");
  fDataHandlersDisabledByType = options.GetValueVector<std::string>("DataHandler.disable-by-type");

  //  Get the globally defined print running sum flag
  fPrintRunningSum = options.GetValue<bool>("print-runningsum");
}

/**
 * Get the handler in this array with the spcified name
 * @param name Name of the handler
 * @return Pointer to the handler
 */
template<typename Handler> VQwDataHandler<Handler>* QwDataHandlerArray<Handler>::GetDataHandlerByName(const TString& name)
{
  VQwDataHandler<Handler>* tmp = NULL;
  if (!empty()) {
    // Loop over the handlers
    for (const_iterator handler = begin(); handler != end(); ++handler) {
      // Check the name of this handler
      // std::cout<<"QwDataHandlerArray::GetDataHandlerByName available name=="<<(*handler)->GetName()<<"== to be compared to =="<<name<<"==\n";
      if ((*handler)->GetName() == name) {
        tmp = (*handler).get();
        //std::cout<<"QwDataHandlerArray::GetDataHandlerByName found a matching name \n";
      } else {
        // nothing
      }
    }
  }
  return tmp;
}


/**
 * Get the list of handlers in this array of the specified type
 * @param type Type of the handler
 * @return Vector of handlers
 */
template<typename Handler> std::vector<VQwDataHandler<Handler>*> QwDataHandlerArray<Handler>::GetDataHandlerByType(const std::string& type)
{
  // Vector of handler pointers
  std::vector<VQwDataHandler<Handler>*> handler_list;

  // If this array is not empty
  if (!empty()) {

    // Loop over the handlers
    for (const_iterator handler = begin(); handler != end(); ++handler) {

      // Test to see if the handler inherits from the required type
      if (VQwDataHandlerFactory<Handler>::InheritsFrom((*handler).get(),type)) {
        handler_list.push_back((*handler).get());
      }

    } // end of loop over handlers

  } // end of if !empty()

  return handler_list;
}

template<typename Handler> void  QwDataHandlerArray<Handler>::ClearEventData()
{
  if (!empty()) {
    std::for_each(begin(), end(),
		  std::mem_fn(&VQwDataHandler<Handler>::ClearEventData));
  }
}



template<typename Handler> void  QwDataHandlerArray<Handler>::ProcessEvent()
{
  if (!empty()){
    std::for_each(begin(), end(), std::mem_fn(&VQwDataHandler<Handler>::ProcessData));
  }
}

template<typename Handler> void  QwDataHandlerArray<Handler>::ConstructTreeBranches(
    QwRootFile *treerootfile,
    const std::string& treeprefix,
    const std::string& branchprefix)
{
  if (!empty()){
    for (iterator handler = begin(); handler != end(); ++handler) {
      handler->get()->ConstructTreeBranches(treerootfile, treeprefix, branchprefix);
    }
  }
}

template<typename Handler> void  QwDataHandlerArray<Handler>::FillTreeBranches(QwRootFile *treerootfile)
{
  if (!empty()){
    for (iterator handler = begin(); handler != end(); ++handler) {
      handler->get()->FillTreeBranches(treerootfile);
    }
  }
}

template<typename Handler> void  QwDataHandlerArray<Handler>::ConstructNTupleFields(
    QwRootFile *treerootfile,
    const std::string& treeprefix,
    const std::string& branchprefix)
{
  if (!empty()){
    for (iterator handler = begin(); handler != end(); ++handler) {
      handler->get()->ConstructNTupleFields(treerootfile, treeprefix, branchprefix);
    }
  }
}

template<typename Handler> void  QwDataHandlerArray<Handler>::FillNTupleFields(QwRootFile *treerootfile)
{
  if (!empty()){
    for (iterator handler = begin(); handler != end(); ++handler) {
      handler->get()->FillNTupleFields(treerootfile);
    }
  }
}



//*****************************************************************

template<typename Handler> void  QwDataHandlerArray<Handler>::ConstructBranchAndVector(TTree *tree, TString& prefix, std::vector <Double_t> &values)
{
  if (!empty()){
    for (iterator handler = begin(); handler != end(); ++handler) {
      VQwDataHandler<Handler>* handler_parity = dynamic_cast<VQwDataHandler<Handler>*>(handler->get());
      handler_parity->ConstructBranchAndVector(tree,prefix,values);
    }
  }
}

template<typename Handler> void  QwDataHandlerArray<Handler>::FillTreeVector(std::vector <Double_t> &values) const
{
  if (!empty()){
    for (const_iterator handler = begin(); handler != end(); ++handler) {
      VQwDataHandler<Handler>* handler_parity = dynamic_cast<VQwDataHandler<Handler>*>(handler->get());
      handler_parity->FillTreeVector(values);
    }
  }
}


//*****************************************************************
template<typename Handler> void  QwDataHandlerArray<Handler>::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  if (!empty()) {
    for (iterator subsys = begin(); subsys != end(); ++subsys){
      (*subsys)->ConstructHistograms(folder,prefix);
    }
  }
}

template<typename Handler> void  QwDataHandlerArray<Handler>::FillHistograms()
{
  if (!empty())
    std::for_each(begin(), end(), std::mem_fn(&VQwDataHandler<Handler>::FillHistograms));
}


//*****************************************************************//

template<typename Handler> void  QwDataHandlerArray<Handler>::FillDB(QwParityDB *db, TString type)
{
  for (iterator handler = begin(); handler != end(); ++handler) {
    VQwDataHandler<Handler>* handler_parity = dynamic_cast<VQwDataHandler<Handler>*>(handler->get());
    handler_parity->FillDB(db, type);
  }
}

/*
template<typename Handler> void  QwDataHandlerArray<Handler>::FillErrDB(QwParityDB *db, TString type)
{
  //  for (const_iterator handler = dummy_source->begin(); handler != dummy_source->end(); ++handler) {
  for (iterator handler = begin(); handler != end(); ++handler) {
    VQwDataHandler<Handler>* handler_parity = dynamic_cast<VQwDataHandler<Handler>*>(handler->get());
    handler_parity->FillErrDB(db, type);
  }
  return;
}
*/

template<typename Handler> void QwDataHandlerArray<Handler>::WritePromptSummary(QwPromptSummary *ps, TString type)
{
  if (!empty()){
    for (const_iterator handler = begin(); handler != end(); ++handler) {
      VQwDataHandler<Handler>* handler_parity = dynamic_cast<VQwDataHandler<Handler>*>(handler->get());
      handler_parity->WritePromptSummary(ps, type);
    }
  }
}


//*****************************************************************//

/**
 * Assignment operator
 * @param source DataHandler array to assign to this array
 * @return This handler array after assignment
 */
template<typename Handler> QwDataHandlerArray& QwDataHandlerArray<Handler>::operator= (const QwDataHandlerArray &source)
{
  Bool_t localdebug=kFALSE;
  if(localdebug)  std::cout<<"QwDataHandlerArray::operator= \n";
  if (!source.empty()){
    if (this->size() == source.size()){
      for(size_t i=0;i<source.size();i++){
	if (source.at(i)==NULL || this->at(i)==NULL){
	  //  Either the source or the destination handler
	  //  are null
	} else {
	  VQwDataHandler<Handler> *ptr1 =
	    dynamic_cast<VQwDataHandler<Handler>*>(this->at(i).get());
          VQwDataHandler<Handler> *ptr2 = source.at(i).get();
	  if (typeid(*ptr1)==typeid(*ptr2)){
	    if(localdebug) std::cout<<" here in QwDataHandlerArray::operator= types mach \n";
	    *(ptr1) = *(source.at(i).get());
	  } else {
	    //  DataHandlers don't match
	      QwError << " QwDataHandlerArray::operator= types do not mach" << QwLog::endl;
	      QwError << " typeid(*ptr1)=" << typeid(*ptr1).name()
                      << " but typeid(*ptr2)=" << typeid(*ptr2).name()
                      << QwLog::endl;
	  }
	}
      }
    } else {
      //  Array sizes don't match
    }
  } else {
    //  The source is empty
  }
  return *this;
}

//*****************************************************************
/*
template<typename Handler> void  QwDataHandlerArray<Handler>::PrintInfo() const
{
  if (!empty()) {
    for (const_iterator handler = begin(); handler != end(); ++handler) {
      (*handler)->PrintInfo();
    }
  }
}
*/
//*****************************************************************//

template<typename Handler> void QwDataHandlerArray<Handler>::PrintValue() const
{
  if (!empty()) {
    for (const_iterator handler = begin(); handler != end(); ++handler) {
      VQwDataHandler<Handler>* handler_parity = dynamic_cast<VQwDataHandler<Handler>*>(handler->get());
      handler_parity->PrintValue();
    }
  }
}




//*****************************************************************//

template<typename Handler> void QwDataHandlerArray<Handler>::CalculateRunningAverage()
{
  if (!empty()) {
    for (iterator handler = begin(); handler != end(); ++handler) {
      VQwDataHandler<Handler>* handler_parity = dynamic_cast<VQwDataHandler<Handler>*>(handler->get());
      handler_parity->CalculateRunningAverage();
    }
  }
}


template<typename Handler> void QwDataHandlerArray<Handler>::AccumulateRunningSum(const QwDataHandlerArray& value, Int_t count, Int_t ErrorMask)
{
  if (!value.empty()) {
    if (this->size() == value.size()) {
	for (size_t i = 0; i < value.size(); i++) {
	  if (value.at(i)==NULL || this->at(i)==NULL) {
	    //  Either the value or the destination handler
	    //  are null
	  } else {
	    VQwDataHandler<Handler> *ptr1 =
	      dynamic_cast<VQwDataHandler<Handler>*>(this->at(i).get());
            VQwDataHandler<Handler> *ptr2 = value.at(i).get();
	    if (typeid(*ptr1) == typeid(*ptr2)) {
	      ptr1->AccumulateRunningSum(*ptr2, count, ErrorMask);
	    } else {
	      QwError << "QwDataHandlerArray::AccumulateRunningSum here where types don't match" << QwLog::endl;
	      QwError << " typeid(ptr1)=" << typeid(ptr1).name()
		      << " but typeid(value.at(i)))=" << typeid(value.at(i)).name()
		      << QwLog::endl;
	      //  DataHandlers don't match
	    }
	  }
	}

    } else {
      //  Array sizes don't match

    }
  } else {
    //  The value is empty
  }
}

template<typename Handler> void QwDataHandlerArray<Handler>::AccumulateAllRunningSum(const QwDataHandlerArray& value, Int_t count, Int_t ErrorMask)
{
  if (!value.empty()) {
    if (this->size() == value.size()) {
	for (size_t i = 0; i < value.size(); i++) {
	  if (value.at(i)==NULL || this->at(i)==NULL) {
	    //  Either the value or the destination handler
	    //  are null
	  } else {
	    VQwDataHandler<Handler> *ptr1 =
	      dynamic_cast<VQwDataHandler<Handler>*>(this->at(i).get());
            VQwDataHandler<Handler> *ptr2 = value.at(i).get();
	    if (typeid(*ptr1) == typeid(*ptr2)) {
	      ptr1->AccumulateRunningSum(*ptr2, count, ErrorMask);
	    } else {
	      QwError << "QwDataHandlerArray::AccumulateRunningSum here where types don't match" << QwLog::endl;
	      QwError << " typeid(ptr1)=" << typeid(ptr1).name()
		      << " but typeid(value.at(i)))=" << typeid(value.at(i)).name()
		      << QwLog::endl;
	      //  DataHandlers don't match
	    }
	  }
	}
    } else {
      //  Array sizes don't match

    }
  } else {
    //  The value is empty
  }
}



/*
template<typename Handler> void QwDataHandlerArray<Handler>::PrintErrorCounters() const{// report number of events failed due to HW and event cut faliure
  const VQwDataHandler<Handler> *handler_parity;
  if (!empty()){
    for (const_iterator handler = begin(); handler != end(); ++handler){
      handler_parity=dynamic_cast<const VQwDataHandler<Handler>*>((handler)->get());
      handler_parity->PrintErrorCounters();
    }
  }
}
*/

/**
 * Add the handler to this array.  Do nothing if the handler is null or if
 * there is already a handler with that name in the array.
 * @param handler DataHandler to add to the array
 */
template<typename Handler> void QwDataHandlerArray<Handler>::push_back(std::shared_ptr<VQwDataHandler<Handler>> handler)
{
  
 if (handler.get() == NULL) {
   QwError << "QwDataHandlerArray::push_back(): NULL handler"
           << QwLog::endl;
   //  This is an empty handler...
   //  Do nothing for now.

 } else if (!this->empty() && GetDataHandlerByName(handler->GetName())){
   //  There is already a handler with this name!
   QwError << "QwDataHandlerArray::push_back(): handler " << handler->GetName()
           << " already exists" << QwLog::endl;

 } else if (!CanContain(handler.get())) {
   //  There is no support for this type of handler
   QwError << "QwDataHandlerArray::push_back(): handler " << handler->GetName()
           << " is not supported by this handler array" << QwLog::endl;

 } else {
   std::shared_ptr<VQwDataHandler<Handler>> handler_tmp(handler);
   HandlerPtrs::push_back(handler_tmp);

/*
   // Set the parent of the handler to this array
   handler_tmp->SetParent(this);


   // Instruct the handler to publish variables
   if (handler_tmp->PublishInternalValues() == kFALSE) {
     QwError << "Not all variables for " << handler_tmp->GetName()
             << " could be published!" << QwLog::endl;
   }
*/
 }
}

/*
template<typename Handler> void QwDataHandlerArray<Handler>::PrintParamFileList() const
{
  if (not empty()) {
    for (const_iterator handler = begin(); handler != end(); ++handler)
      {
        (*handler)->PrintDetectorMaps(true);
      }
  }
}

template<typename Handler> TList* QwDataHandlerArray<Handler>::GetParamFileNameList(TString name) const
{
  if (not empty()) {

    TList* return_maps_TList = new TList;
    return_maps_TList->SetName(name);

    std::map<TString, TString> mapfiles_handler;

    for (const_iterator handler = begin(); handler != end(); ++handler) 
      {
	mapfiles_handler = (*handler)->GetDetectorMaps();
	for( std::map<TString, TString>::iterator ii= mapfiles_handler.begin(); ii!= mapfiles_handler.end(); ++ii)
	  {	
	    TList *test = new TList;
	    test->SetName((*ii).first);
	    test->AddLast(new TObjString((*ii).second));
	    return_maps_TList -> AddLast(test);
	  }
      }

    return return_maps_TList;
  }
  else {
    return NULL;
  }
};

*/

template<typename Handler> void QwDataHandlerArray<Handler>::ProcessDataHandlerEntry()
{
  if (!empty()) {
    for(iterator handler = begin(); handler != end(); ++handler){
      (*handler)->ProcessData();
      (*handler)->AccumulateRunningSum();
    }
  }
}

template<typename Handler> void QwDataHandlerArray<Handler>::FinishDataHandler()
{
  if (!empty()) {
    for(iterator handler = begin(); handler != end(); ++handler){
      (*handler)->FinishDataHandler();
    }
  }
}
template class QwDataHandlerArray<QwSubsystemArrayParity>;
template class QwDataHandlerArray<QwHelicityPattern>;
