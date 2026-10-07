#pragma once
#include "EpicHandler.h"
#include "EpicChannel.h"
#include "ChannelObserver.h"
#include "Publisher.h"
#include "QwFeedbackLogger.h"
#include "EpicTypes.h"
#include <atomic>
enum class IHWP {
    kIN  = 0, // PER EPICS
    kOUT = 1  // PER EPICS                                                                                
};
std::string stringify(IHWP ihwp);
class IHWP_IOC : public EPICSObserver<int>, public Publisher<QwFeedbackLogPayload>
{
	EpicChannel* fChannel;
	AtomicEpicsType<int> fCurrState;
public:
	static const char* IHWP_PV;
	void Update(int const& data) override;
    IHWP GetState() const;
public:
	IHWP_IOC();
	IHWP_IOC(EpicChannel* channel);
	~IHWP_IOC();
	IHWP_IOC(IHWP_IOC const& other);
	IHWP_IOC(IHWP_IOC&& other) = delete;
	IHWP_IOC& operator=(IHWP_IOC const& other) = delete;
	IHWP_IOC& operator=(IHWP_IOC&& other) = delete;
};    
