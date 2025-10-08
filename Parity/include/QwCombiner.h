/*
 * QwCombiner.h
 *
 *  Created on: Oct 22, 2010
 *      Author: wdconinc
 *
 *  Last Modified: August 1, 2018 1:45 PM
 */

#ifndef QWCOMBINER_H_
#define QWCOMBINER_H_

// Parent Class
#include "VQwDataHandler.h"


template<typename Type>
class QwCombiner:public VQwDataHandler<Type>, public MQwDataHandlerCloneable<Type, QwCombiner<Type>>
{
 public:
    typedef std::vector< VQwHardwareChannel* >::iterator Iterator_HdwChan;
    typedef std::vector< VQwHardwareChannel* >::const_iterator ConstIterator_HdwChan;

 public:
    /// \brief Constructor with name
    QwCombiner(const TString& name);

    /// \brief Copy constructor
    QwCombiner(const QwCombiner &source);
    /// Virtual destructor
    virtual ~QwCombiner();

    /// \brief Load the channels and sensitivities
    Int_t LoadChannelMap(const std::string& mapfile);

    /// \brief Connect to Channels (event only)
    Int_t ConnectChannels(QwSubsystemArrayParity& event);
    /// \brief Connect to Channels (asymmetry/difference only)
    Int_t ConnectChannels(QwSubsystemArrayParity& asym,
			  QwSubsystemArrayParity& diff);

    void ProcessData();
  
  protected:
  
    /// Default constructor (Protected for child class access)
    QwCombiner() { };

    /// Error flag mask
    UInt_t fErrorFlagMask;

    /// List of channels to use in the combiner
    std::vector< std::vector< typename VQwDataHandler<Type>::EQwHandleType > > fIndependentType;
    std::vector< std::vector< std::string > > fIndependentName;
    std::vector< std::vector< const VQwHardwareChannel* > > fIndependentVar;
    std::vector< std::vector< Double_t > > fSensitivity;

	/* Dependent Name Aliases */
	using VQwDataHandler<Type>::VQwDataHandler;
	using VQwDataHandler<Type>::ParseSeparator;
	using VQwDataHandler<Type>::fKeepRunningSum;
	using VQwDataHandler<Type>::fErrorFlagPtr;
	using VQwDataHandler<Type>::fOutputVar;
	using VQwDataHandler<Type>::fDependentVar;
	using VQwDataHandler<Type>::fDependentType;
	using VQwDataHandler<Type>::fDependentName;
	using VQwDataHandler<Type>::fDependentFull;
	using VQwDataHandler<Type>::fName;
	/* This is the ideal way to access the enum class
	   but it is a c++20 feature
	using VQwDataHandler<QwHelicityPattern>::EQwHandleType  ;
	using VQwDataHandler<QwHelicityPattern>::EQwHandleType::kHandleTypeMps ;
	using VQwDataHandler<QwHelicityPattern>::EQwHandleType::kHandleTypeAsym;
	using VQwDataHandler<QwHelicityPattern>::EQwHandleType::kHandleTypeDiff;
	*/





}; // class QwCombiner

template<typename Type>
inline std::ostream& operator<< (std::ostream& stream, const typename QwCombiner<Type>::EQwHandleType& i) {
  switch (i){
  case VQwDataHandler<Type>::EQwHandleType::kHandleTypeMps:  stream << "mps"; break;
  case VQwDataHandler<Type>::EQwHandleType::kHandleTypeAsym: stream << "asym"; break;
  case VQwDataHandler<Type>::EQwHandleType::kHandleTypeDiff: stream << "diff"; break;
  default:           stream << "Unknown";
  }
  return stream;
}


#endif // QWCOMBINER_H_
