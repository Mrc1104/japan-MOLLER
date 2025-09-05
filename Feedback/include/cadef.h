#ifndef CADEF_H
#define CADEF_H

#include <string>
#include <string_view>
#include <fstream>
#include <map>
#include <sstream>
#include <iostream>
#include <memory>
#include "TString.h"

#include "epicsTypes.h"
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

// Path to DB
static char *const ARCHIVE_DATA_BASE = "/home/mrc/logfiles/fake_epics/archive/%s_archive.txt";
static char *const OUTPUT_DATA_BASE  = "/home/mrc/logfiles/fake_epics/output/%s_output.txt";
static std::map<std::string, std::unique_ptr<std::ifstream>> archive_db_istream;
static std::map<std::string, std::unique_ptr<std::ofstream>> output_db_ostream;

enum TYPES
{
	// Data types
	DBR_DOUBLE,
	DBR_STRING,
	DBR_INT
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
	// check to see if <ioc, std::stream> exist
	if(archive_db_istream.count(ioc)==0) { // not found, create the stream
		archive_db_istream[ioc] = std::make_unique<std::ifstream>(Form(ARCHIVE_DATA_BASE, ioc.c_str()));
	}
	auto stream = std::move(archive_db_istream[ioc]);
	
	// try to read in (value could be a double or string)
	// we'll use stringstream to handle the overloaded case
	unsigned counter = 0, max = 1;
	std::string stream_string{""};
	do{
		counter++;
		if(stream->eof()) {
			std::cout << "Reading DB for IOC: " << ioc << " hit EOF... Rewinding\n";
			stream->clear();
			stream->seekg(0);
		}
		
		getline(*stream, stream_string);
		if(!stream->good()) {
			std::cout << "Reading DB for IOC: " << ioc << " failed\n";
			stream_string= "";
		}

	} while( (stream->eof()) || (counter < max) ) ;

	if( stream_string != "" ) {
		std::stringstream stream_entry(stream_string);
		if constexpr(std::is_same<decltype(stream_string.c_str()), const T*>::value) {
			for(unsigned c = 0; c < stream_string.size(); c++)
				stream_entry >> value[c];
		}
		else {
			stream_entry >> *value;
		}
	} else {
		*value = static_cast<T>(0x3F); // set it eqaul to '?'
	}
	archive_db_istream[ioc] = std::move(stream);	
	return 0;
}

template<class T>
inline int ca_put(const TYPES type, const chid &ioc, T* value)
{
	// check to see if <ios, ostream> exists
	if(output_db_ostream.count(ioc) == 0) { // not found, create the stream
		output_db_ostream[ioc] = std::make_unique<std::ofstream>(Form(OUTPUT_DATA_BASE, ioc.c_str()));
	}
	auto stream = std::move(output_db_ostream[ioc]);
	// we create this file so lot less error checking
	if constexpr(std::is_same<const char*, const T*>::value) {
		for(unsigned c = 0; c < MAX_STRING_SIZE; c++) {
			if(value[c] == '\0') break;
			*stream << value[c];
		}
		*stream << '\n';
	}
	else {
		*stream << *value << '\n';
	}

	output_db_ostream[ioc] = std::move(stream);	
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
