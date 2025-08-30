#ifndef CADEF_H
#define CADEF_H

#include <string>
#include <string_view>

/*
	DO NOT USE OTHER THAN FOR TESTING


	Name: cadef.h
	Author: Ryan Conaway
	Purpose: This is a fake cadef.h (originally an EPICS source file)
	         I do not have an EPICS system setup, no do I need a full one.
			 The feedback system only uses three EPICS calls:
			   - ca_get()     -- Gets the value of a IOC from the EPICS system
			   - ca_put()     -- Stores the value into the EPICS IOC
			   - ca_pend_io() -- Determines if we wait for the io to complete,
			                     and for how long
			 This file shadows those functions. To use, include
		         'using namespace FAKE_EPICS;' 
             at the top of QwEPICSControl.h.
*/

typedef std::string chid;
// Channel ID (chid), 'bind' this to a IOC
// by calling 'ca_search("IOC_NAME", chid)

namespace FAKE_EPICS
{

enum TYPES
{
	// Data types
	DBR_DOUBLE,
	DBR_STRING,
};

enum channel_state {
    /// valid chid, IOC not found or unavailable
	cs_never_conn,
    /// valid chid, IOC was found, but unavailable (previously connected to server)
	cs_prev_conn,
    /// valid chid, IOC was found, still available
	cs_conn,
    /// channel deleted by user
	cs_closed
};

template<class T>
inline int ca_get(const TYPES type, const chid &ioc, T *value)
{
	return 0;
}

template<class T>
inline int ca_put(const TYPES type, const chid &ioc, T* value)
{
	return 0;
}

inline int ca_pend_io(const unsigned &val)
{
	// Always return success
	return 0;
}

inline int ca_search(std::string s, chid *channel_id)
{
	// Excellent talk by Sandeep Mantrala, "Understanding C++ Value Categories"
	// moves if possible; just learned this -mrc
	*channel_id = std::move(s);
	return 0;
}

inline channel_state ca_state(chid chan)
{
	return cs_conn;
}

}; // namespace FAKE_EPICS
#endif
