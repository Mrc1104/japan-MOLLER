#ifndef Podd_THaEtClient_h_
#define Podd_THaEtClient_h_

//////////////////////////////////////////////////////////////////////
//
//   THaEtClient
//   Data from ET Online System
//
//   THaEtClient contains normal CODA data obtained via
//   the ET (Event Transfer) online system invented
//   by the JLab DAQ group.
//   This code works locally or remotely and uses the
//   ET system in a particular mode favored by  hall A.
//
//   Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaCodaData.h"
// #include "evetLib.h"
#include <ctime>
#include <cstring>
#include <stdlib.h>
#include <byteswap.h>

//#define ET_CHUNK_SIZE 50
#define ET_CHUNK_SIZE 1
#ifndef __CINT__
#include "et.h"
#endif

// Bryan Moffit's Command Line Options
struct ETClientOptions_t
{
  int              flowMode=ET_STATION_SERIAL, position=ET_END, pposition=ET_END;
  int              chunk=1, qSize=0;
	bool						 remote=0, blocking=1, dump=0;
  bool             multicast=0, broadcast=0, broadAndMulticast=0, noDelay=0, wait=0;
  int              sendBufSize=0, recvBufSize=0;
  int              debugLevel = ET_DEBUG_ERROR;
  unsigned short   port=0;
  char             stationName[ET_STATNAME_LENGTH], et_name[ET_FILENAME_LENGTH], host[256], interface[16];

	int							 mcastAddrCount = 0; // Orignal code supported up to 10 mcastAddresses.
	char             mcastAddr[16]; 
	ETClientOptions_t(){
		memset(host, 0, 256);
		memset(interface, 0, 16);
		memset(mcastAddr, 0, 16);
		memset(et_name, 0, ET_FILENAME_LENGTH);
		memset(stationName, 0, ET_STATNAME_LENGTH);
	}
	int setStationName(const char* string, size_t stringSize){
		if( stringSize >= ET_STATNAME_LENGTH ) return 1;
		strcpy(stationName, string);
		return 0;
	}
	int setETName(const char* string, size_t stringSize){
		if( stringSize >= ET_FILENAME_LENGTH ) return 1;
		strcpy(et_name, string);
		return 0;
	}
	int setHostName(const char* string, size_t stringSize){
		if( stringSize >= 255) return 1;
		strcpy(host, string);
		return 0;
	}
	int setInterfaceAddr(const char* string, size_t stringSize){
		if( stringSize > 15 || stringSize < 7 ) return 1;
		strcpy(interface, string);
		return 0;
	}
	int setMcastAddr(const char* string, size_t stringSize){
		if( stringSize > 15 ) return 1;
		strcpy(mcastAddr, string);
		mcastAddrCount++;
		return 0;
	}
};


class TString;
class THaEtClient : public THaCodaData
{

public:

   Int_t codaOpen(const char* file_name, Int_t mode=1) { return 0; }
   Int_t codaOpen(const char* file_name, const char* session, Int_t mode=1) {return 0;}
		THaEtClient(const ETClientOptions_t* config);
    ~THaEtClient();

    Int_t codaClose();
    Int_t codaRead();            // codaRead() must be called once per event
    virtual bool isOpen() const;

// Attributes of et_event from et_event_getdata
typedef struct etChunkStat
{
  uint32_t *data;
  size_t length;
  int32_t endian;
  int32_t swap;

  int32_t  evioHandle;
} etChunkStat_t;

typedef struct evetHandle
{
  et_sys_id etSysId;
  et_att_id etAttId;
  et_event **etChunk;      // pointer to array of et_events (pe)
  int32_t  etChunkSize;    // user requested (et_events in a chunk)
  int32_t  etChunkNumRead; // actual read from et_events_get

  int32_t  currentChunkID;  // j
  etChunkStat_t currentChunkStat; // data, len, endian, swap

  int32_t verbose;

} evetHandle_t ;

int32_t  evetOpen(et_sys_id etSysId, int32_t chunk, evetHandle_t &evh);
int32_t  evetClose(evetHandle_t &evh);
int32_t  evetReadNoCopy(evetHandle_t &evh, const uint32_t **outputBuffer, uint32_t *length);

int32_t evetGetEtChunks(evetHandle_t &evh);
int32_t evetGetChunk(evetHandle_t &evh);
private:

    THaEtClient(const THaEtClient &fn);
    THaEtClient& operator=(const THaEtClient &fn);
    Int_t timeout;
    char *daqhost,*session,*etfile;
		Int_t didclose,notopened,firstread;
    int init();

// rate calculation
    Int_t firstRateCalc;
    Int_t evsum, xcnt;
    time_t daqt1;
    double ratesum;

	// Bryan Moffit's Option Parameters
	const ETClientOptions_t *userConfig;
  char localAddr[16];
	int locality;
	
	// event handle
	evetHandle evh;

	// ET Station Configuration
	char host[256];
	int port;
  et_stat_id      my_stat;
  et_statconfig   sconfig;
  et_openconfig   openconfig;
  /* statistics variables */
  int64_t totalBytes=0;
  int32_t evCount = 0;

};


#endif
