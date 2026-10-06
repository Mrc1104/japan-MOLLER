#include "IHWP.h"

const char* IHWP_IOC::IHWP_PV = "IGL1I00DI24_24M";

IHWP IHWP_IOC::GetState() const
{
	int value = fCurrState.Load(std::memory_order_acquire);
	return (value == static_cast<int>(IHWP::kIN)) ? IHWP::kIN : IHWP::kOUT;
}

void IHWP_IOC::Update(int const& data)
{
	fCurrState.Store(data, std::memory_order_release);
}


IHWP_IOC::IHWP_IOC()
: IHWP_IOC(EpicHandler::Instance().ConnectChannel(IHWP_IOC::IHWP_PV))
{ }

IHWP_IOC::IHWP_IOC(EpicChannel* channel)
: fChannel{channel}
, fCurrState{0}
{
	fChannel->StartMonitoring(this);
}
IHWP_IOC::~IHWP_IOC()
{
	fChannel->StopMonitoring(this);
}

IHWP_IOC::IHWP_IOC(IHWP_IOC const& other)
: fChannel{other.fChannel}
, fCurrState{0}
{
	fCurrState.Store(other.fCurrState.Load(std::memory_order_relaxed),
						std::memory_order_relaxed);
	fChannel->StartMonitoring(this);
}

