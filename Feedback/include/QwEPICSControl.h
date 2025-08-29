#ifndef __QW_EPICS_CONTROL__
#define __QW_EPICS_CONTROL__

#include <string>
#include "cadef.h"
using namespace FAKE_EPICS;


/*
 * QwEPICS
 *	This abstracts the EPICS calls (namely the cadef calls)
 *	into one class. The class contains one variable, the chid,
 *	which is linked to the corresponding EPICS var through a
 *	call to 'ca_search(<name>)'. The <name> is stored in the
 *	EPICS db used by JLab.
 *
 * Discussion:
 *	Ideally, this class will read be instantiated by passing
 *	the name of the EPICS var to search (ie. ca_search(<name>)
 *	will be called in the constructor. What happens if the string
 *	passed into the arg is not found in the db? 
 *	Likely, we should quit the program if a single IOC fails (how
 *	do we do feedback without the feedback channels?).
 *
 *	Alternative, the constructor takes a valid chid object (user 
 *	is responsible for calling ca_search and handling failure)?
 */
class QwEPICS
{
public:
	QwEPICS(std::string ioc_name);
	~QwEPICS();

public:
	// Wrappers wound the cadef commands
	// Read the EPICS db
	void read(std::string &ret);
	void read(double &ret);
	// Write to the EPICS db
	void write(std::string val);
	void write(double val);
	// Sync for IO operations
	void sync();

private:
	// Check to see if the IOC connected successfully.
	bool connected();
private:
	chid ioc;
	TYPES
}
#endif
