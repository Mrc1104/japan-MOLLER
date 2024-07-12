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
#include <ctime>
#include <cstring>
#include <stdlib.h>

//#define ET_CHUNK_SIZE 50
#define ET_CHUNK_SIZE 1
#ifndef __CINT__
#include "et.h"
#endif

class TString;

// The ET memory file will have this prefix.  The suffix is $SESSION.
#define ETMEM_PREFIX "/tmp/et_sys_"

//FIXME: un-hardcode these ... sigh
#define ADAQL1 "129.57.164.53"
#define ADAQL2 "129.57.164.59"
#define ADAQEP "129.57.164.78"
#define ADAQCP "129.57.164.79"
#define ADAQS2 "129.57.164.44"
#define ADAQS3 "129.57.164.45"

// Bryan Moffit's Command Line Options
struct ETClientOptions_t
{
  int              flowMode=ET_STATION_SERIAL, position=ET_END, pposition=ET_END;
  int              chunk=1, qSize=0;
	bool						 remote=0, blocking=1, dump=0;
  bool             multicast=0, broadcast=0, broadAndMulticast=0, noDelay=0;
  int              sendBufSize=0, recvBufSize=0;
  unsigned short   port=0;
  char             stationName[ET_STATNAME_LENGTH], et_name[ET_FILENAME_LENGTH], host[256], interface[16];

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
		return 0;
	}
};

class THaEtClient : public THaCodaData
{

public:

    explicit THaEtClient(Int_t mode=1);   // By default, gets data from ADAQS2
// find data on 'computer'.  e.g. computer="129.57.164.44"
    explicit THaEtClient(const char* computer, Int_t mode=1);
    THaEtClient(const char* computer, const char* session, Int_t mode=1, const char* stationname="japan_sta");
    ~THaEtClient();

    Int_t codaOpen(const char* computer, Int_t mode=1);
    Int_t codaOpen(const char* computer, const char* session, Int_t mode=1);
    Int_t codaClose();
    Int_t codaRead();            // codaRead() must be called once per event
    virtual bool isOpen() const;

private:

    THaEtClient(const THaEtClient &fn);
    THaEtClient& operator=(const THaEtClient &fn);
    Int_t nread, nused, timeout;
#ifndef __CINT__
    et_sys_id id;
    et_att_id my_att;
#endif
    char *daqhost,*session,*etfile;
    Int_t waitflag,didclose,notopened,firstread;
    Int_t init(const char* computer="japan_sta");

// rate calculation
    Int_t firstRateCalc;
    Int_t evsum, xcnt;
    time_t daqt1;
    double ratesum;

	// dynamic station name support
	const char* defaultStationName = "japan_sta";
	char stationName[ET_STATNAME_LENGTH] = "japan_sta";

	// Bryan Moffit's Option Parameters
  char localAddr[16];
	int errflg=0;

};


#endif
