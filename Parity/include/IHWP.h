#pragma once
#include "EpicChannel.h"
#include <atomic>
enum class IHWP {
    kIN  = 0, // PER EPICS
    kOUT = 1  // PER EPICS                                                                                
};

class IHWP_IOC
{
	EpicChannel* ioc;
	std::atomic<short> val;
private:
	static void monitor_callback(event_handler_args arg) noexcept;
	IHWP convert_to_ihwp(short val) const;
public:
	IHWP_IOC(EpicChannel *chan);
	~IHWP_IOC();
	IHWP_IOC(IHWP_IOC const&) = delete;
	IHWP_IOC& operator=(IHWP_IOC const&) = delete;
	// TODO: Implement move operations
	IHWP_IOC(IHWP_IOC &&) = delete;
	IHWP_IOC& operator=(IHWP_IOC &&) = delete;
public:
    IHWP GetState() const;
};    
