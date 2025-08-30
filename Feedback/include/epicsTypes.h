#ifndef EPICSTYPES_H
#define EPICSTYPES_H
/*
	DO NOT USE OTHER THAN FOR TESTING


	Name: epicsTypes.h
	Author: Ryan Conaway
	Purpose: This is a fake epicsTYPES.h (originally an EPICS source file)
	         I do not have an EPICS system setup, no do I need a full one.
			 I only need one definition from epicsTypes.h
			   - MAX_STRING_SIZE (set to 40 characters)
			 This file shadows those functions. To use, include
		         'using namespace FAKE_EPICS;' 
             at the top of QwEPICSControl.h.
*/

namespace FAKE_EPICS
{
	static const unsigned MAX_STRING_SIZE = 40;
}; // namespace FAKE_EPICS

#endif
