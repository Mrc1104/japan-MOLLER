#include "VEventDecoder.h"
#include "QwOptions.h"

Bool_t VEventDecoder::DecodeSubbankHeader(UInt_t *buffer){
	//  This function will decode the header information from
	//  either a ROC bank or a subbank.  It will also bump
	//  fWordsSoFar to be referring to the first word of
	//  the subbank's data.
	//
	//  NOTE TO DAQ PROGRAMMERS:
	//      All internal subbank tags MUST be defined to
	//      be greater than 31.
	Bool_t okay = kTRUE;
	if (fWordsSoFar >= fEvtLength){
		//  We have reached the end of this event.
		okay = kFALSE;
	} else if (fBankDataType == 0x10) {
		//  This bank has subbanks, so decode the subbank header.
		fFragLength   = buffer[0] - 1;  // This is the number of words in the data block
		fSubbankTag   = (buffer[1]&0xFFFF0000)>>16; // Bits 16-31
		fSubbankType  = (buffer[1]&0xFF00)>>8;      // Bits 8-15
		fSubbankNum   = (buffer[1]&0xFF);           // Bits 0-7

		QwDebug << "QwEventBuffer::DecodeSubbankHeader: "
			<< "fROC=="<<fROC << ", fSubbankTag==" << fSubbankTag
			<< ", fSubbankType=="<<fSubbankType << ", fSubbankNum==" <<fSubbankNum
			<< ", fAllowLowSubbankIDs==" << fAllowLowSubbankIDs
			<< QwLog::endl;

		if (fSubbankTag<=31 
			&& ( (fAllowLowSubbankIDs==kFALSE)
			|| (fAllowLowSubbankIDs==kTRUE && fSubbankType==0x10) ) ){
			//  Subbank tags between 0 and 31 indicate this is
			//  a ROC bank.
			fROC        = fSubbankTag;
			fSubbankTag = 0;
		}
		if (fWordsSoFar+2+fFragLength > fEvtLength){
			//  Trouble, because we'll have too many words!
			QwError << "fWordsSoFar+2+fFragLength=="<<fWordsSoFar+2+fFragLength
					<< " and fEvtLength==" << fEvtLength
					<< QwLog::endl;
			okay = kFALSE;
		}
		fWordsSoFar   += 2;
	}
	QwDebug << "QwEventBuffer::DecodeSubbankHeader: " 
			<< "fROC=="<<fROC << ", fSubbankTag==" << fSubbankTag <<": "
			<<  std::hex
			<< buffer[0] << " "
			<< buffer[1] << " "
			<< buffer[2] << " "
			<< buffer[3] << " "
			<< buffer[4] << std::dec << " "
			<< fWordsSoFar << " "<< fEvtLength
			<< QwLog::endl;
	//  There is no final else, because any bank type other than 
	//  0x10 should just return okay.
	return okay;
}

// Prints out all the internal member variables
// Args: QwLog (QwMessage, QwWarning, or QwError)
void VEventDecoder::PrintDecoderInfo(QwLog& out)
{
	out << "\n-------\n" << std::hex <<
		"fWordsSoFar " << fWordsSoFar <<
		"\n fEvtLength; " << fEvtLength <<
		"\n fEvtType " << fEvtType <<
		"\n fEvtTag " << fEvtTag <<
		"\n fBankDataType " << fBankDataType <<
		"\n fPhysicsEventFlag " << fPhysicsEventFlag <<
		"\n fEvtNumber;   " << fEvtNumber <<
		"\n fFragLength " << fFragLength <<
		"\n fSubbankTag " << fSubbankTag <<
		"\n fSubbankType " << fSubbankType <<
		"\n fSubbankNum " << fSubbankNum <<
		"\n fROC " << fROC <<
		"\n fAllowLowSubbankIDs " << fAllowLowSubbankIDs <<
		"\n-------\n" << std::dec <<
		QwLog::endl;
}


Int_t VEventDecoder::DecodeETStream(UInt_t *buffer)
{
	assert(buffer);
	QwMessage << "Now this is podracing!" << QwLog::endl;
	for(int i = 0; i < buffer[0]; i++)
	{
		if( i % 4 == 0) QwMessage << "\n" << std::dec << i << std::hex << "\t";
		QwMessage << std::hex <<  buffer[i] << " ";
	}
	QwMessage << std::dec << QwLog::endl;

	return 0;
}


/*
	Decodes the bit info from the Network Transfor (ET) Format.
  Comment: Given an ET Evio Block Header, this expects bit 8-14 from
					 the 5th word (counting from 0).
*/
void VEventDecoder::DecodeETBitInfo(UInt_t bitInfo)
{
	ETBitInfo.hasDictionary =  (bitInfo & 0x1);
	ETBitInfo.isLastBlock   = ((bitInfo & 0x2)  >> 1 );
	ETBitInfo.isFirstEvent  = ((bitInfo & 0x40) >> 6 );
	ETBitInfo.payloadType   = ((bitInfo & 0x3C) >> 2 );
	ETBitInfo.ETPrint(QwMessage);
}

/*
	Resets the ET Bit Info Values to their init state
*/
void VEventDecoder::ETBitInfo_t::ETReset()
{
	payloadType   = -1;
	hasDictionary = kFALSE;
	isLastBlock   = kFALSE;
	isFirstEvent  = kFALSE;
}
/*
	Prints out the ET Bit Info Verbosely
*/
void VEventDecoder::ETBitInfo_t::ETPrint(QwLog& out)const
{
	out << std::boolalpha << "Et has Dictionary: " << hasDictionary
			<< "\n ET has Last Block: " << isLastBlock << "\nEt has First Event: " << isFirstEvent
			<< "\n ET Payload Type: " << sETType() << std::noboolalpha << QwLog::endl;
}
/*
	Converts the ET Payload type from hex to string
*/
string VEventDecoder::ETBitInfo_t::sETType()const
{
	string ret;
	switch(payloadType){
	case ROCRaw:
		ret = "ROC";
		break;
	case PHYS:
		ret = "PHYS";
		break;
	case PartialPHYS:
		ret = "Partial Phys";
		break;
	case Disentangled:
		ret = "Disentangled";
		break;
	case User:
		ret = "USER";
		break;
	case Control:
		ret = "CONTROL";
		break;
	case Other:
		ret = "OTHER";
		break;
	default:
		ret = "DEFAULT";
		break;
	}
	return ret;
}

// Calls the Appropriate Decoding Scheme
// Comment: DecodePHYSPayload is the default for File Decoding
Int_t VEventDecoder::DecodeEventIDBank(UInt_t *buffer)
{
	Int_t status = CODA_OK;
	switch(ETBitInfo.payloadType){
	case ROCRaw:
		status = DecodeROCRawPayload(buffer);
		break;
	case PartialPHYS:
		status = DecodePartialPHYSPayload(buffer);
		break;
	case Disentangled:
		status = DecodeDisentangledPayload(buffer);
		break;
	case User:
		status = DecodeUserPayload(buffer);
		break;
	case Control:
		status = DecodeControlPayload(buffer);
		break;
	case Other:
		status = DecodeOtherPayload(buffer);
		break;
	case PHYS:
	default:
		status = DecodePHYSPayload(buffer);
		break;
	}
	return status;
}

Int_t VEventDecoder::DecodeROCRawPayload(UInt_t *buffer)
{
	QwWarning << "Decoding ROC Raw Payloads is not implemented!" << QwLog::endl;
	return kFALSE;
}
Int_t VEventDecoder::DecodePHYSPayload(UInt_t *buffer)
{
	QwWarning << "Decoding PHYS Payloads is not implemented!" << QwLog::endl;
	return kFALSE;
}
Int_t VEventDecoder::DecodePartialPHYSPayload(UInt_t *buffer)
{
	QwWarning << "Decoding Partial PHYS Payloads is not implemented!" << QwLog::endl;
	return kFALSE;
}
Int_t VEventDecoder::DecodeDisentangledPayload(UInt_t *buffer)
{
	QwWarning << "Decoding Disentangled Payloads is not implemented!" << QwLog::endl;
	return kFALSE;
}
Int_t VEventDecoder::DecodeUserPayload(UInt_t *buffer)
{
	QwWarning << "Decoding USER Payloads is not implemented!" << QwLog::endl;
	return kFALSE;
}
Int_t VEventDecoder::DecodeControlPayload(UInt_t *buffer)
{
	QwWarning << "Decoding Control Payloads is not implemented!" << QwLog::endl;
	return kFALSE;
}

Int_t VEventDecoder::DecodeOtherPayload(UInt_t *buffer)
{
	QwWarning << "Decoding Other Payloads is not implemented!" << QwLog::endl;
	return kFALSE;
}
