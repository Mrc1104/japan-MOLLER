#ifndef VEVENTDECODER_H
#define VEVENTDECODER_H

/**********************************************************\
* File: VEventDecoder.h                                    *
*                                                          *
* Author:                                                  *
* Time-stamp:                                              *
* Description: Contains the base functions needed to       *
*              Encode and Decode CODA data and distribute  *
*              to the subsystems
\**********************************************************/

#include <vector>


#include "Rtypes.h"
#include "QwTypes.h"
#include "MQwCodaControlEvent.h"
#include "QwOptions.h"


/* TODO:
 * Inheriting from MQwCodaControlEvent here solves our
 * problem of scope for the keywords and ProcessControl() calls;
 * however, this muddies the class because we no longer are just (en)decoding
 * the event header but also are now processing control events too.
 * Should we redesign?
 * 	-> Make a seperate KEYWORD header?
 * 	-> Make a "fControlEvent" flag then handle control events inside QwEventBuffer?
 */
class VEventDecoder : public MQwCodaControlEvent {
public:
	VEventDecoder() : 
		fWordsSoFar(0),
		fEvtLength(0),
		fEvtNumber(0),
		fFragLength(0),
		fEvtType(0),
		fEvtTag(0),
		fBankDataType(0),
		fSubbankTag(0),
		fSubbankType(0),
		fSubbankNum(0),
		fROC(0),
    	fPhysicsEventFlag(kFALSE),
		fControlEventFlag(kFALSE),
		fAllowLowSubbankIDs(kFALSE) { }

	virtual ~VEventDecoder()  { }

public:
	// Encoding Functions
	virtual std::vector<UInt_t> EncodePHYSEventHeader() = 0;
	virtual void EncodePrestartEventHeader(int* buffer, int runnumber, int runtype, int localtime) = 0;
  	virtual void EncodeGoEventHeader(int* buffer, int eventcount, int localtime)     = 0;
	virtual void EncodePauseEventHeader(int* buffer, int eventcount, int localtime)  = 0;
	virtual void EncodeEndEventHeader(int* buffer, int eventcount, int localtime)    = 0;

public:
	// Decoding Functions
	virtual Int_t DecodeEventIDBank(UInt_t *buffer) = 0;
	virtual Bool_t DecodeSubbankHeader(UInt_t *buffer);
	virtual void PrintDecoderInfo(QwLog& out);

public:
	// Boolean Functions
	virtual Bool_t IsPhysicsEvent() = 0;
	virtual Bool_t IsROCConfigurationEvent(){
		return (fEvtType>=0x90 && fEvtType<=0x18f);
	};

	virtual Bool_t IsEPICSEvent(){
		return (fEvtType==EPICS_EVTYPE); // Defined in CodaDecoder.h
	}


// TODO:
// Make these data members protected!
public:
	// Information we need to extact from the decoding
	// How to expose this info to QwEventBuffer..?
	// Maybe make a seperate data class and give the pointer to the decoder? 
	// Data_t* pdata
	// Or should we just use getter's and setters?
	// Generic Information
	UInt_t fWordsSoFar;
	UInt_t fEvtLength;	
	UInt_t fEvtNumber;   ///< CODA event number; only defined for physics events
	UInt_t fFragLength;

	// Event Information
	UInt_t fEvtType;
	UInt_t fEvtTag;
	UInt_t fBankDataType;
	BankID_t fSubbankTag;
	UInt_t fSubbankType;
	UInt_t fSubbankNum;
	ROCID_t fROC;

	// Booleans
	Bool_t fPhysicsEventFlag;
	Bool_t fControlEventFlag;
	Bool_t fAllowLowSubbankIDs;
protected:
	enum KEYWORDS {
		EPICS_EVTYPE = 131
	};

};

#endif
