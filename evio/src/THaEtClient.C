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

#include "../include/THaEtClient.h"
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <ctime>        // for timespec
#include <stdexcept>
#include "evio.h"       // for evioswap
#include "et_private.h" // for ET_VERSION
// #include "evetLib.h"

using namespace std;

static const int FAST          = 25;
static const int SMALL_TIMEOUT = 10;
static const int BIG_TIMEOUT   = 20;

#define EVETCHECKINIT(x)					\
  if(x.etSysId == 0) {						\
    printf("%s: ERROR: evet not initiallized\n", __func__);	\
    return -1;}

// Common member initialization for our constructors
#define initflags                                       \
timeout(BIG_TIMEOUT),               \
didclose(0), notopened(0), firstread(1),   \
firstRateCalc(1), evsum(0), xcnt(0), daqt1(-1), ratesum(0)

THaEtClient::THaEtClient(const ETClientOptions_t* config)
	: initflags
{
	cout << "Hello";
	cin.get();
	userConfig = config;
	memset(host, 0, 256);
	init();
}

THaEtClient::~THaEtClient() {
  delete [] daqhost;
  delete [] session;
  delete [] etfile;
  Int_t status = THaEtClient::codaClose();
  if (status == CODA_ERROR) cout << "ERROR: closing THaEtClient"<<endl;
}
Int_t THaEtClient::init( )
{
	int status;
	et_open_config_init(&openconfig);
  /* if multicasting to find ET */
	if(userConfig->multicast) {
		if( userConfig->mcastAddrCount < 1) {
			/* Use default mcast address if not given */
			status = et_open_config_addmulticast(openconfig, ET_MULTICAST_ADDR);
		}
		else {
			/* add multicast addresses to use */
			/* Original code supported up to 10 mcastAddr */
			if(strlen(userConfig->mcastAddr)>7){
				status = et_open_config_addmulticast(openconfig, userConfig->mcastAddr);
				if(status != ET_OK) {
					printf("bad multicast address argument\n");
					exit(1);
				}
				printf("Adding multicast address %s\n",  userConfig->mcastAddr);
			}
		}
	}

	if(userConfig->broadAndMulticast){
		port = userConfig->port;
		printf("Broad and Multicasting\n");
		if (port == 0){
			port = ET_UDP_PORT;
		}
		et_open_config_setport(openconfig, port);
		et_open_config_setcast(openconfig, ET_BROADANDMULTICAST);
		et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
	}
	else if (userConfig->multicast) {
		port = userConfig->port;
		printf("Multicasting\n");	
		if (port == 0){
			port = ET_UDP_PORT;
		}
		et_open_config_setport(openconfig, port);
		et_open_config_setcast(openconfig, ET_MULTICAST);
		et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
	}
	else if (userConfig->broadcast) {
		port = userConfig->port;
		printf("Broadcasting\n");
		if (port == 0) {
			port = ET_UDP_PORT;
		}
		et_open_config_setport(openconfig, port);
		et_open_config_setcast(openconfig, ET_BROADCAST);
		et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
	}
	else {
		port = userConfig->port;
		if (port == 0) {
			port = ET_SERVER_PORT;
		}
		et_open_config_setserverport(openconfig, port);
		et_open_config_setcast(openconfig, ET_DIRECT);
		if (strlen(userConfig->host) > 0) {
			et_open_config_sethost(openconfig, userConfig->host);
		}
		et_open_config_gethost(openconfig, host);
    printf("Direct connection to %s\n", host);
	}

  /* Defaults are to use operating system default buffer sizes and turn off TCP_NODELAY */
	et_open_config_settcp(openconfig, userConfig->recvBufSize, userConfig->sendBufSize, userConfig->noDelay);	
	if (strlen(userConfig->interface) > 6) {
		et_open_config_setinterface(openconfig, userConfig->interface);
	}

	if (userConfig->remote) {
		printf("(Set as remote\n");
		et_open_config_setmode(openconfig, ET_HOST_AS_REMOTE);
	}

	/* If reponses from different ET systems, return error */
	et_open_config_setpolicy(openconfig, ET_POLICY_ERROR);

	/* debug level */
	et_open_config_setdebugdefault(openconfig, userConfig->debugLevel);
	et_open_config_setwait(openconfig, ET_OPEN_WAIT);
	et_station_config_setprescale(openconfig, 1);
	et_station_config_setselect(openconfig, ET_STATION_SELECT_ALL);

	et_sys_id id = 0;
	if (et_open(&id, userConfig->et_name, openconfig) != ET_OK) {
		printf("%s: et_open problems\n",__func__);
		return -1;
	}

	if (userConfig->debugLevel == ET_DEBUG_INFO) {
		evh.verbose = 1;	
	}
	else {
		evh.verbose = 0;
	}
	evetOpen(id, userConfig->chunk, evh);

	et_open_config_destroy(openconfig);

  /*-------------------------------------------------------*/
	
	/* Find out if we have a remote connection to the ET system */
	et_system_getlocality(evh.etSysId, &locality);
	if (locality == ET_REMOTE) {
		printf("ET is remote\n\n");
		et_system_gethost(evh.etSysId, host);
		et_system_getlocaladdress(evh.etSysId, localAddr);
    printf("Connect to ET, from ip = %s to %s\n", localAddr, host);
	}
	else {
		printf("ET is local\n\n");
	}

	/* set level of debug output (everything) */
	et_system_setdebug(evh.etSysId, userConfig->debugLevel);

	/* define station to create */
	et_station_config_init(&sconfig);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT); // Do we want to give access to USER?
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI); // Do we want to give access to USER?
	et_station_config_setflow(sconfig, userConfig->flowMode);
	if (!userConfig->blocking) {
		et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
		if (userConfig->qSize > 0) {
			et_station_config_setcue(sconfig,userConfig->qSize);
		}
	}
	
	if (( status = 
				 et_station_create_at(evh.etSysId, &my_stat, userConfig->stationName, sconfig, userConfig->position, userConfig->pposition) ) != ET_OK) {

		if ( status == ET_ERROR_EXISTS) {
			/* my_stat contains pointer exisiting station */
			printf("%s: station already exists\n", userConfig->stationName);
		}
		else if (status == ET_ERROR_TOOMANY) {
			printf("too many stations created\n");
		}
		else {
			printf("error in station creation\n");
		}
	}
	et_station_config_destroy(sconfig);
	if ((status = et_station_attach(evh.etSysId, my_stat, &evh.etAttId)) != ET_OK) {
		printf("error in station attach\n");
	}

	return status;
}

Int_t THaEtClient::codaClose() {
  if (didclose || firstread) return CODA_OK;
  didclose = 1;
  if (notopened) return CODA_ERROR;
  if (et_station_detach(evh.etSysId, evh.etAttId) != ET_OK) {
    cout << "ERROR: codaClose: detaching from ET"<<endl;
    return CODA_ERROR;
  }
  if (et_close(evh.etSysId) != ET_OK) {
    cout << "ERROR: codaClose: error closing ET"<<endl;
    return CODA_ERROR;
  }
  return CODA_OK;
}

Int_t THaEtClient::codaRead()
{
  const size_t bpi = sizeof(uint32_t);

  if (firstRateCalc) {
     firstRateCalc = 0;
     daqt1 = time(nullptr);
  }
  else {
  	time_t daqt2 = time(nullptr);
    double tdiff = difftime(daqt2, daqt1);
		evsum += evh.etChunkNumRead;
    if ((tdiff > 4) && (evsum > 30)) {
	 		double daqrate  = static_cast<double>(evsum)/tdiff;
      evsum  = 0;
      ratesum += daqrate;
      double avgrate  = ratesum/++xcnt;

      if (verbose > 0) {
      	printf("ET rate %4.1f Hz in %2.0f sec, avg %4.1f Hz\n", daqrate, tdiff, avgrate);
      }
      	if (userConfig->wait != 0) {
      		timeout = (avgrate > FAST) ? SMALL_TIMEOUT : BIG_TIMEOUT;
        }
        daqt1 = time(nullptr);
    }
  }

	int status;
	const uint32_t *readBuffer;
	uint32_t len;
	status = evetReadNoCopy(evh, &readBuffer, &len);
	evCount++;
	totalBytes += len;
	if(status == 0){
		/*
	  printf("evetRead(%2d): \n", ++evCount);
	  uint32_t i;
	  for (i=0; i< (len); i++) {
	    printf("0x%08x ", readBuffer[i]);
	    if( (i+1) % 8 == 0)
	      printf("\n");
	  }
	  printf("\n");
		*/

		if( !evbuffer.grow(len/bpi+1) )
			throw runtime_error("THaEtClient: Maximum event buffer size reached");
		assert(bpi * evbuffer.size() >= (size_t)len);
		// printf("NUMBER OF BYTES TO COPY: %i\n", len);
		memcpy(evbuffer.get(), readBuffer, sizeof(uint32_t)*len);
	}

  return status;
}

bool THaEtClient::isOpen() const {
  return (notopened==1&&didclose==0);
}

int32_t
THaEtClient::evetOpen(et_sys_id etSysId, int32_t chunk, evetHandle_t &evh)
{
  evh.etSysId = etSysId;
  evh.etChunkSize = chunk;

  evh.etAttId = 0;
  evh.currentChunkID = -1;
  evh.etChunkNumRead = -1;

  evh.currentChunkStat.evioHandle = 0;
  evh.currentChunkStat.length = 0;
  evh.currentChunkStat.endian = 0;
  evh.currentChunkStat.swap = 0;

  /* allocate some memory */
  evh.etChunk = (et_event **) calloc((size_t)chunk, sizeof(et_event *));
  if (evh.etChunk == NULL) {
    printf("%s: out of memory\n", __func__);
    evh.etSysId = 0;
    return -1;
  }

  return 0;
}

int32_t
THaEtClient::evetClose(evetHandle_t &evh)
{

	// Close up any currently opened evBufferOpen's.
	if(evh.currentChunkStat.evioHandle)
	{
		int32_t stat = evClose(evh.currentChunkStat.evioHandle);
		if(stat != S_SUCCESS)
		{
			printf("%s: ERROR: evClose returned %s\n",
					__func__, et_perror(stat));
			return -1;
		}
	}

	// put any events we may still have
	if(evh.etChunkNumRead != -1)
	{
		/* putting array of events */
		int32_t status = et_events_put(evh.etSysId, evh.etAttId, evh.etChunk, evh.etChunkNumRead);
		if (status != ET_OK)
		{
			printf("%s: ERROR: et_events_put returned %s\n",
					__func__, et_perror(status));
			return -1;
		}
	}

	// free up the etChunk memory
	if(evh.etChunk)
		free(evh.etChunk);

	return 0;
}


int32_t
THaEtClient::evetGetEtChunks(evetHandle_t &evh)
{
	if(evh.verbose == 1)
		printf("%s: enter\n", __func__);

	EVETCHECKINIT(evh);

	int32_t status;
	if (userConfig->wait == 0) {
		status = et_events_get(evh.etSysId, evh.etAttId, evh.etChunk, ET_SLEEP, NULL, evh.etChunkSize, &evh.etChunkNumRead);
	}
	else {
		struct timespec twait{};
		twait.tv_sec  = timeout;
		twait.tv_nsec = 0;
		status = et_events_get(evh.etSysId, evh.etAttId, evh.etChunk, ET_TIMED, &twait, evh.etChunkSize, &evh.etChunkNumRead);
	}
	cout << "NUMBER OF CHUNKS REQUESTED: " << evh.etChunkSize;
	cout << "\n NUMBER OF CHUNKS RECEIVED: " << evh.etChunkNumRead << endl;
	if(status != ET_OK)
	{
		printf("%s: ERROR: et_events_get returned (%d) %s\n",
				__func__, status, et_perror(status));
		if (status == ET_ERROR_TIMEOUT) {
			printf("et_netclient: timeout calling et_events_get\n");
			printf("Probably means CODA is not running...\n");
		}
	}

	evh.currentChunkID = -1;

	return status;
}

int32_t
THaEtClient::evetGetChunk(evetHandle_t &evh)
{
	if(evh.verbose == 1)
		printf("%s: enter\n", __func__);

	EVETCHECKINIT(evh);

	evh.currentChunkID++;

	if((evh.currentChunkID >= evh.etChunkNumRead) || (evh.etChunkNumRead == -1))
	{
		if(evh.etChunkNumRead != -1)
		{
			/* putting array of events */
			int32_t status = et_events_put(evh.etSysId, evh.etAttId, evh.etChunk, evh.etChunkNumRead);
			if (status != ET_OK)
			{
				printf("%s: ERROR: et_events_put returned %s\n",
						__func__, et_perror(status));
				return -1;
			}
		}

		// out of chunks.  get some more
		int32_t stat = evetGetEtChunks(evh);
		if(stat != 0)
		{
			printf("%s: ERROR: evetGetEtChunks(evh) returned %d\n",
					__func__, stat);
			return -1;
		}
		evh.currentChunkID++;

	}

	// Close previous handle
	if(evh.currentChunkStat.evioHandle)
	{
		int32_t stat = evClose(evh.currentChunkStat.evioHandle);
		if(stat != ET_OK)
		{
			printf("%s: ERROR: evClose returned %s\n",
					__func__, et_perror(stat));
			return -1;
		}
	}

	et_event *currentChunk = evh.etChunk[evh.currentChunkID];
	et_event_getdata(currentChunk, (void **) &evh.currentChunkStat.data);
	et_event_getlength(currentChunk, &evh.currentChunkStat.length);
	et_event_getendian(currentChunk, &evh.currentChunkStat.endian);
	et_event_needtoswap(currentChunk, &evh.currentChunkStat.swap);

	if(evh.verbose == 1)
	{
		uint32_t *data = evh.currentChunkStat.data;
		uint32_t idata = 0, len = evh.currentChunkStat.length;

		printf("data byte order = %s\n",
				(evh.currentChunkStat.endian == ET_ENDIAN_BIG) ? "BIG" : "LITTLE");
		printf(" %2d/%2d: data (len = %d) %s  int = %d\n",
				evh.currentChunkID, evh.etChunkNumRead,
				(int) len,
				evh.currentChunkStat.swap ? "needs swapping" : "does not need swapping",
				evh.currentChunkStat.swap ? bswap_32(data[0]) : data[0]);

		for(idata = 0; idata < ((32 < (len>>2)) ? 32 : (len>>2)); idata++)
		{
			printf("0x%08x ", evh.currentChunkStat.swap ? bswap_32(data[idata]) : data[idata]);
			if(((idata+1) % 8) == 0)
				printf("\n");
		}
		printf("\n");
	}

	int32_t evstat = evOpenBuffer((char *) evh.currentChunkStat.data, evh.currentChunkStat.length,
			(char *)"r",  &evh.currentChunkStat.evioHandle);

	if(evstat != 0)
	{
		printf("%s: ERROR: evOpenBuffer returned %s\n",
				__func__, et_perror(evstat));
	}

	return evstat;
}

int32_t
THaEtClient::evetReadNoCopy(evetHandle_t &evh, const uint32_t **outputBuffer, uint32_t *length)
{
	if(evh.verbose == 1)
		printf("%s: enter\n", __func__);

	EVETCHECKINIT(evh);

	int32_t status = evReadNoCopy(evh.currentChunkStat.evioHandle,
																outputBuffer, length);
	if(status != S_SUCCESS)
	{
		// Get a new chunk from et_get_event
		status = evetGetChunk(evh);
		if(status == 0)
		{
			status = evReadNoCopy(evh.currentChunkStat.evioHandle,
														outputBuffer, length);
		}
		else
		{
			printf("%s: ERROR: evetGetChunk failed %d\n",
					__func__, status);
		}
	}

	return status;
}
