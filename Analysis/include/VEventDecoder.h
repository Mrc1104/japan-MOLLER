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
	virtual std::vector<UInt_t> EncodePHYSEventHeader(std::vector<ROCID_t> &ROCList) = 0;
	virtual void EncodePrestartEventHeader(int* buffer, int runnumber, int runtype, int localtime) = 0;
  virtual void EncodeGoEventHeader(int* buffer, int eventcount, int localtime)     = 0;
	virtual void EncodePauseEventHeader(int* buffer, int eventcount, int localtime)  = 0;
	virtual void EncodeEndEventHeader(int* buffer, int eventcount, int localtime)    = 0;

public:
	// Decoding Functions
	virtual Int_t DecodeEventIDBank(UInt_t *buffer) = 0;
	virtual Int_t DecodeROCRawPayload(UInt_t *buffer);
	virtual Int_t DecodePHYSPayload(UInt_t *buffer);
	virtual Int_t DecodePartialPHYSPayload(UInt_t *buffer);
	virtual Int_t DecodeDisentangledPayload(UInt_t *buffer);
	virtual Int_t DecodeUserPayload(UInt_t *buffer);
	virtual Int_t DecodeControlPayload(UInt_t *buffer);
	virtual Int_t DecodeOtherPayload(UInt_t *buffer);
	virtual Int_t DecodeETStream(UInt_t *buffer);
	virtual void DecodeETBitInfo(UInt_t bitInfo);
	virtual Bool_t DecodeSubbankHeader(UInt_t *buffer);
	virtual void PrintDecoderInfo(QwLog& out);

public:
	// Boolean Functions
	virtual Bool_t IsPhysicsEvent(){
		return fPhysicsEventFlag;
	}

	virtual Bool_t IsROCConfigurationEvent(){
		return (fEvtType>=0x90 && fEvtType<=0x18f);
	}
	
	virtual Bool_t IsControlEvent(){
		return fControlEventFlag;
	}

	virtual Bool_t IsEPICSEvent(){
		return (fEvtType==EPICS_EVTYPE); // Defined in CodaDecoder.h
	}
// Accessor Functions
	// Generic Information
	UInt_t GetWordsSoFar()const  { return fWordsSoFar;  }
	UInt_t GetEvtNumber()const   { return fEvtNumber;   }
	UInt_t GetFragLength()const  { return fFragLength;  }
	// Event Information
	UInt_t GetEvtType()const     { return fEvtType;     }
	UInt_t GetBankDataType()const{ return fBankDataType;}
	UInt_t GetSubbankTag()const  { return fSubbankTag;  }
	UInt_t GetSubbankType()const { return fSubbankType; }
	ROCID_t GetROC()const        { return fROC;         }

// Mutator Functions
	void SetWordsSoFar(UInt_t val)                  { fWordsSoFar = val;         }
	void AddWordsSoFarAndFragLength()               { fWordsSoFar += fFragLength;}
	void SetFragLength(UInt_t val)									{ fFragLength = val;		  	 }
	void SetAllowLowSubbankIDs(Bool_t val = kFALSE) { fAllowLowSubbankIDs = val; }
	
public:
	struct ETBitInfo_t
	{
		unsigned char payloadType;
		Bool_t hasDictionary;
		Bool_t isLastBlock;
		Bool_t isFirstEvent;
		void ETReset();
		void ETPrint(QwLog& out)const;
		string sETType()const;
		ETBitInfo_t() : payloadType(0), hasDictionary(kFALSE),
										isLastBlock(kFALSE), isFirstEvent(kFALSE) { }
	};

protected:
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

	// ET info
	ETBitInfo_t ETBitInfo;

protected:
	enum KEYWORDS {
		// General Event Types
		EPICS_EVTYPE = 131,
		// ET Payload Types
		ROCRaw = 0,
		PHYS = 1,
		PartialPHYS = 2,
		Disentangled = 3,
		User = 4,
		Control = 5,
		Other = 15
	};

};

#endif
