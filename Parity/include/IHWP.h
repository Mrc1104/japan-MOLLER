#pragma once
#include "EpicHandler.h"
#include "EpicChannel.h"
#include <atomic>
enum class IHWP {
    kIN  = 0, // PER EPICS
    kOUT = 1  // PER EPICS                                                                                
};

class IHWP_IOC
{
	// WARNING:
	// Requires EpicHandler to be constructed first / destructed second
	// Cross-references meyers singleton perserves that
	EpicHandler& epics;
	EpicChannel* ioc;
	std::atomic<short> val;
private:
	static const char* IHWP_PV;
	static void monitor_callback(event_handler_args arg) noexcept;
	IHWP convert_to_ihwp(short val) const;
private:
	IHWP_IOC();
	~IHWP_IOC();
	IHWP_IOC(IHWP_IOC const&) = delete;
	IHWP_IOC& operator=(IHWP_IOC const&) = delete;
	IHWP_IOC(IHWP_IOC &&) = delete;
	IHWP_IOC& operator=(IHWP_IOC &&) = delete;
public:
	static IHWP_IOC& getInstance();
public:
    IHWP GetState() const;
};    
