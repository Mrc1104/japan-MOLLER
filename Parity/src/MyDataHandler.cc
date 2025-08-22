#include "MyDataHandler.h"

// Qweak headers
#include "VQwDataElement.h"
#include "QwVQWK_Channel.h"
#include "QwParameterFile.h"
#include "QwHelicityPattern.h"

#include "QwMollerADC_Channel.h"

#define MYSQLPP_SSQLS_NO_STATICS
#ifdef __USE_DATABASE__
#include "QwParitySSQLS.h"
#include "QwParityDB.h"
#endif // __USE_DATABASE__


// Register this handler with the factory
RegisterHandlerFactory(MyDataHandler);

/// \brief Constructor with name
MyDataHandler::MyDataHandler(const TString& name)
: VQwDataHandler(name)
{
	ParseSeparator = "_";
	fKeepRunningSum = kTRUE;
	fErrorFlagMask = 0;
	fErrorFlagPointer = 0;
}

MyDataHandler::MyDataHandler(const MyDataHandler &source)
: VQwDataHandler(source)
{
	fErrorFlagMask = 0;
	fErrorFlagPointer = 0;
}

/// Destructor
MyDataHandler::~MyDataHandler() {}


// I need to overload this because the default VQwDataHandler::LoadChannelMao(mapfile)
// just returns 0 (fails quietly)
Int_t MyDataHandler::LoadChannelMap(const std::string& mapfile)
{
	// Open the Parameter File
	QwParameterFile map(mapfile);

	// Read the sections of dependent variables
	std::pair<EQwHandleType, std::string> type_name;
	
	// Loop over mapfile and look for keys
	// Format: [label] [type]_[name]
	//    ie :     dv    asym_tq05r2
	while(map.ReadNextLine()) {
		// Remove comments, spaces, and empty lines.
		map.TrimComment();
		map.TrimWhitespace();
		if(map.LineIsEmpty()) continue;

		// Get first token: dv or iv; and second token [asym, diff, mps]
		std::string primary_token = map.GetNextToken(" ");
		std::string current_token = map.GetNextToken(" ");

		type_name = ParseHandledVariable(current_token);

		if(primary_token == "iv") {
			fIndependentType.push_back( type_name.first  );
			fIndependentName.push_back( type_name.second );
			fIndependentFull.push_back(  current_token   );
		} else if(primary_token == "dv") {
			fDependentType.push_back( type_name.first  );
			fDependentName.push_back( type_name.second );
			fDependentFull.push_back(  current_token   );
		} else if(primary_token == "treetype") {
			QwMessage << "Tree Type read, ignoring." << QwLog::endl;
		} else {
			QwError << "LoadChannelMan in QwCorrelator read invalid primary token: " << primary_token << QwLog::endl;
		}
	}
	/* For Reassurance -- mrc */
	for(auto const & v : fIndependentType)
		QwMessage << v << QwLog::endl;
	for(auto const & v : fIndependentName)
		QwMessage << v << QwLog::endl;
	for(auto const & v : fIndependentFull)
		QwMessage << v << QwLog::endl;
	for(auto const & v : fDependentType)
		QwMessage << v << QwLog::endl;
	for(auto const & v : fDependentName)
		QwMessage << v << QwLog::endl;
	for(auto const & v : fDependentFull)
		QwMessage << v << QwLog::endl;

	return 0;
}

// For Feedback, we will not need this. Leave this with an empty implementation
Int_t MyDataHandler::ConnectChannels(QwSubsystemArrayParity& event)
{
	SetEventcutErrorFlagPointer(event.GetEventcutErrorFlagPointer());
	// Return if handler is not enabled

	// Fill vector of pointers to the relevant data elements
	// This is how we "share" access


	// Get dependent vars first
	for(size_t dv = 0; dv <fDependentName.size(); dv++) {
		// Skip Asym and Diff types
		// ( they should call ConnectChannels(QwSubsystemArrayParity& asym,QwSubsystemArrayParity& asym) )
		if(fDependentType.at(dv) == kHandleTypeAsym || fDependentType.at(dv) == kHandleTypeDiff) continue;
		// We also want to avoid Unknown Types (display warning)
		if(fDependentType.at(dv) != kHandleTypeMps) {
			QwWarning << "MyDataHandler::ConnectChannels(QwSubsystemArrayParity& event): Dependent variable, "
			          << fDependentName.at(dv)
					  << ", for does not hvae MPS type. Type == "
					  << fDependentType.at(dv)
					  << "." << QwLog::endl;
			continue;
		}
		const VQwHardwareChannel *dv_ptr = this->RequestExternalPointer(fDependentFull.at(dv));
		if(dv_ptr == nullptr) {
			dv_ptr = event.RequestExternalPointer(fDependentName.at(dv));
		}
		if(dv_ptr == nullptr) {
			QwWarning << "MyDataHandler::ConnectChannels(QwSubsystemArrayParity& asym,QwSubsystemArrayParity& diff): Dependent variable, "
			          << fDependentName.at(dv)
					  << " was not found (fullname == "
					  << fDependentFull.at(dv) << QwLog::endl;
			continue;
		}
		// pair creation
		if(dv_ptr != nullptr) {
			fDependentVar.push_back(dv_ptr);
			fOutputVar.emplace_back( dv_ptr->Clone() );
		}
	}


	// Get independent vars
	for(size_t iv = 0; iv <fIndependentName.size(); iv++) {
		const VQwHardwareChannel *iv_ptr = this->RequestExternalPointer(fIndependentFull.at(iv));
		if(iv_ptr == nullptr) {
			iv_ptr = event.RequestExternalPointer(fIndependentName.at(iv));
		}
		if(iv_ptr == nullptr) {
			QwWarning << "MyDataHandler::ConnectChannels(QwSubsystemArrayParity& asym,QwSubsystemArrayParity& diff): Independent variable, "
			          << fIndependentName.at(iv)
					  << " was not found (fullname == "
					  << fIndependentFull.at(iv) << QwLog::endl;
			continue;
		}
		// pair creation
		if(iv_ptr != nullptr) {
			fIndependentVar.push_back(iv_ptr);
			fOutputVar_other.emplace_back( iv_ptr->Clone() );
		}
	}
	fIndependentValues.resize(fIndependentVar.size());
	fDependentValues.resize(fDependentVar.size());

	fOutputValues.resize(fOutputVar.size());
	fOutputValues_other.resize(fOutputVar_other.size());

	return 0;
}
Int_t MyDataHandler::ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
{
	SetEventcutErrorFlagPointer(asym.GetEventcutErrorFlagPointer());
	// Return if handler is not enabled
	// How is this handled?

	// Fill vector of pointers to the relevant data elements
	// This is how we "share" access

	// Get dependent vars first
	for(size_t dv = 0; dv < fDependentName.size(); dv++) {
		if( fDependentType.at(dv) == kHandleTypeMps) {
			// Ignore MPS type when connecting to asymm or diff
			continue;
		}
		
		const VQwHardwareChannel *dv_ptr = this->RequestExternalPointer(fDependentFull.at(dv));
		if(dv_ptr == nullptr) {
			switch(fDependentType.at(dv)) {
			case kHandleTypeAsym:
				dv_ptr = asym.RequestExternalPointer(fDependentName.at(dv));
				break;
			case kHandleTypeDiff:
				dv_ptr = asym.RequestExternalPointer(fDependentName.at(dv));
				break;
			default:
				QwWarning << "MyDataHandler::ConnectChannels(QwSubsystemArrayParity& asym,QwSubsystemArrayParity& diff): Dependent variable, "
				          << fDependentName.at(dv)
						  << " was not found (fullname == "
						  << fDependentFull.at(dv) << QwLog::endl;
				break;
			}
		} // end if(dv_ptr == nullptr)

		// pair creation
		if(dv_ptr != nullptr) {
			fDependentVar.push_back(dv_ptr);
			fOutputVar.emplace_back(  dv_ptr->Clone(VQwDataElement::kDerived) );
		} else {
			QwWarning << "MyDataHandler::ConnectChannels(QwSubsystemArrayParity& asym,QwSubsystemArrayParity& diff): Dependent variable, "
				<< fDependentName.at(dv)
				<< " was not found (fullname == "
				<< fDependentFull.at(dv) << QwLog::endl;
		}


	}

	// Add Independent Vars
	for(size_t iv = 0; iv < fIndependentName.size(); iv++) {
		// Get the independent variables
		const VQwHardwareChannel *iv_ptr = this->RequestExternalPointer(fIndependentFull.at(iv));
		if(iv_ptr == nullptr) {
			switch(fIndependentType.at(iv)) {
			case kHandleTypeAsym:
				iv_ptr = asym.RequestExternalPointer(fIndependentName.at(iv));
				printf("iv_ptr -> %p\n", iv_ptr);
				break;
			case kHandleTypeDiff:
				iv_ptr = diff.RequestExternalPointer(fIndependentName.at(iv));
				printf("iv_ptr -> %p\n", iv_ptr);
				break;
			default:
				QwWarning << "Independent variable for MyDataHandler has unknown type." << QwLog::endl;
				break;
			}
		}

		if(iv_ptr != nullptr) {
			// pair creation
			fIndependentVar.push_back(iv_ptr);
			fOutputVar_other.emplace_back( iv_ptr->Clone(VQwDataElement::kDerived) );
		} else {
			QwWarning << "Independent variable "
			          << fIndependentName.at(iv)
					  << " for MyDataHandler could not be found."
					  << QwLog::endl;
		}
	}

	fIndependentValues.resize(fIndependentVar.size());
	fDependentValues.resize(fDependentVar.size());

	fOutputValues.resize(fOutputVar.size());
	fOutputValues_other.resize(fOutputVar_other.size());

	return 0;
}

/*  Just use the base class version for now... */
/*  Might need to overload to allow for parsing of 'ep' data types */
void MyDataHandler::ParseConfigFile(QwParameterFile& file)
{
	VQwDataHandler::ParseConfigFile(file);
}


void MyDataHandler::ProcessData()
{
	// I can see the data just find through this method --mrc 08/15/25
	for (size_t i = 0; i < fDependentVar.size(); ++i) {
		*(dynamic_cast<QwMollerADC_Channel*>(fOutputVar.at(i))) = *(dynamic_cast<const QwMollerADC_Channel*>(fDependentVar[i]) );

	}
	for (size_t i = 0; i < fDependentValues.size(); ++i) {
		fOutputValues.at(i) = fDependentValues[i];
	}
	for (size_t i = 0; i < fIndependentVar.size(); ++i) {
		*(dynamic_cast<QwMollerADC_Channel*> (fOutputVar_other.at(i))) = *(dynamic_cast<const QwMollerADC_Channel*>( fIndependentVar[i]) );
		QwMessage << "fIndependentVar: " << fIndependentVar[i]->GetValue(1) << "\n";;
		QwMessage << "fOutputVar_other: " << fOutputVar_other[i]->GetValue(1) << "\n";
	}
	for (size_t i = 0; i < fIndependentValues.size(); ++i) {
		fOutputValues_other.at(i) = fIndependentValues[i];
	}
}

// Implement Tree Logic
void MyDataHandler::ConstructTreeBranches(QwRootFile *treerootfile, const std::string& treeprefix,
        				   const std::string& branchprefix)
{
	if(fTreeName == "") {
    	QwWarning << "MyDataHandler: no tree name specified, use 'tree-name = value'" << QwLog::endl;
    	QwWarning << "Using MyDataHandler as Tree name." << QwLog::endl;
		fTreeName = "MyDataHandler";
	}
	if( (branchprefix.find("stat") != string::npos) && fKeepRunningSum && fRunningsum != NULL ) {
			fRunningsumFillsTree = kTRUE;
	} else {
			fRunningsumFillsTree = kFALSE;
	}

	// Construct Tree name and create new tree
	fTreeName = treeprefix + fTreeName;
	if(fRunningsumFillsTree) {
		treerootfile->ConstructTreeBranches(fTreeName, fTreeComment, *fRunningsum, fPrefix+branchprefix);
	} else {
		for(auto const &output : fOutputVar) {
			treerootfile->ConstructTreeBranches(fTreeName, fTreeComment.c_str(), *output);
		}
		for(auto const &output : fOutputVar_other) {
			treerootfile->ConstructTreeBranches(fTreeName, fTreeComment.c_str(), *output);
		}
	}

}
void MyDataHandler::FillTreeBranches(QwRootFile *treerootfile)
{

    if (fRunningsumFillsTree) {
      treerootfile->FillTreeBranches(*fRunningsum);
	} else {

		for(auto const &output : fOutputVar)
			treerootfile->FillTreeBranches(*output);
		for(auto const &output : fOutputVar_other)
			treerootfile->FillTreeBranches(*output);
	}
    treerootfile->FillTree(fTreeName);
}
/**
 * Fill the tree vector
 * @param values Vector of values
 */
void MyDataHandler::FillTreeVector(std::vector<Double_t>& values) const
{

  // Fill the data element
  for (size_t i = 0; i < fOutputVar.size(); ++i) {
    if (fOutputVar.at(i) == NULL) {continue;}
    fOutputVar.at(i)->FillTreeVector(values);
  }

/*
  // Fill the data element
  for (size_t i = 0; i < fOutputVar.size(); ++i) {
    if (fOutputVar.at(i) == NULL) {continue;}
    fOutputVar.at(i)->FillTreeVector(values);
  }
  for (size_t i = 0; i < fOutputVar_other.size(); ++i) {
    if (fOutputVar_other.at(i) == NULL) {continue;}
    fOutputVar_other.at(i)->FillTreeVector(values);
  }
  */
}


