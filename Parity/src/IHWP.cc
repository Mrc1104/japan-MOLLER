#include "IHWP.h"

IHWP_IOC::IHWP_IOC(EpicChannel *chan)
: ioc(chan)
, val{0}
{
	ioc->StartMonitoring(DBR_TIME_SHORT, monitor_callback, this);
}

IHWP_IOC::~IHWP_IOC()
{
	if(ioc) {
		ioc->StopMonitoring();
	}
}

void IHWP_IOC::monitor_callback(event_handler_args arg) noexcept
{
	void* private_data = arg.usr;
	if( arg.status == ECA_NORMAL && arg.dbr != nullptr && private_data != nullptr) {
        auto* instance = static_cast<IHWP_IOC*>(private_data);
		auto const* data = static_cast<dbr_time_short const*>( arg.dbr );
		instance->val.store(data->value, std::memory_order_release);
		std::cout << "[Update] Channel '" << ::ca_name(arg.chid)
		          << "' status changed: "
				  << (instance->convert_to_ihwp(data->value) == IHWP::kIN ? "IN\n" : "OUT\n");
	}
}

IHWP IHWP_IOC::convert_to_ihwp(short val) const
{
	return (val == static_cast<short>(IHWP::kIN)) ? IHWP::kIN : IHWP::kOUT;
}
IHWP IHWP_IOC::GetState() const
{
	short state = val.load(std::memory_order_acquire);
	return convert_to_ihwp(state);
}
