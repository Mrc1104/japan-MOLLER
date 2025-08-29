#include "QwEPICSControl.h"

#include <string>
#include <stdexcept>

#include "QwLog.h"
#include "cadef.h"
#include "epicsTypes.h" // define MAX_STRING_SIZE

static const unsigned SYNC_WAIT_DURATION = 10;

// What happens if the constructor fails?
QwEPICS::QwEPICS(std::string ioc_name)
{
	// Change this to QwDebug
	QwMessage << "Connecting to IOC Channel: " << ioc_name << QwLog::endl;
	ca_search(ioc_name, ioc);
	// Do I need to call ca_pend_io first before checking its connection?
	// ca_pend_io(10)
	if( !connected() )
	{
		throw std::invalid_argument("Could not connect to IOC Channel\n");
	}
}


void QwEPICS::read(std::string &ret)
{
 	// EPICS DB API returns null-terminated c-strings
	char buffer[MAX_STRING_SIZE] = {0};
	ca_get(DBR_STRING, ioc, buffer);

	// does this copy the buffer twice?
	// ret = std::string(buffer);
	// Tested it, std::move is faster
	ret = std::move(buffer);
}
void QwEPICS::read(double &ret)
{
	ca_get(DBR_DOUBLE, ioc, &ret);
}

void QwEPICS::write(std::string &val)
{
	ca_put(DBR_STRING, ioc, val.c_str());
}
void QwEPICS::sync()
{
	ca_pend_io(SYNC_WAIT_DURATION);
}

// caget.h
//enum channel_state {
//    /// valid chid, IOC not found or unavailable
//    cs_never_conn,
//    /// valid chid, IOC was found, but unavailable (previously connected to server)
//    cs_prev_conn,
//    /// valid chid, IOC was found, still available
//    cs_conn,
//    /// channel deleted by user
//    cs_closed
//};
bool QwEPICS::connected()
{
	bool is_connected = kFALSE;
	channel_state state = ca_state(ioc);
	switch (state) {
	case cs_never_conn:
		QwError << "IOC not found or unavailable" << QwLog::endl;
		is_connected = kFALSE;
	break;
	case cs_prev_conn:
		QwError << "IOC was found, but unavailable (previously connected to server)" << QwLog::endl;
		is_connected = kFALSE;
	break;
	case cs_conn:
		QwError << "Valid chid, IOC was found, still available" << QwLog::endl;
		is_connected = kTRUE;
	break;
	case cs_closed:
		QwError << "Channel deleted by user" << QwLog::endl;
		is_connected = kFALSE;
	break;
	default:
		QwError << "Unknown IOC" << QwLog::endl;
		is_connected = kFALSE;
	};
	return is_connected;
}

// How to cleanup a chid object?
QwEPICS::~QwEPICS(){}
