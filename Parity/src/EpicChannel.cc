#include "EpicChannel.h"
void EpicChannel::connection_callback(connection_handler_args arg) noexcept {
	std::string_view ch_name = ::ca_name(arg.chid);
	std::cout << "[Callback] Channel '" << ch_name << "' status changed: ";
	if(arg.op == CA_OP_CONN_UP) {
		std::cout << "Connected to IOC (Host: " << ::ca_host_name(arg.chid) <<")\n";
	} else if( arg.op == CA_OP_CONN_DOWN) {
		std::cout << "Disconnected from IOC\n";
	}
    if (void* private_data = ::ca_puser(arg.chid)) {
        auto* instance = static_cast<EpicChannel*>(private_data);
        instance->is_conn.store(arg.op == CA_OP_CONN_UP);
    }
}

void EpicChannel::monitor_callback(event_handler_args arg) noexcept
{
	if( arg.status == ECA_NORMAL && arg.dbr != nullptr ) {
		auto const* data = static_cast<dbr_time_string const*>( arg.dbr );
		std::cout << ">>> Live PV Update: " << data->value << '\n';
	}
}

void EpicChannel::async_get_callback(::event_handler_args arg)
{
	std::unique_ptr<AsyncContextBase> context(
		static_cast<AsyncContextBase*>(arg.usr));
	if(!context) return;
	context->Fulfill(arg.status, arg.type, arg.dbr);
}
void EpicChannel::async_put_callback(::event_handler_args arg)
{
	std::unique_ptr<AsyncContextBase> context(
		static_cast<AsyncContextBase*>(arg.usr));
	if(!context) return;
	context->Fulfill(arg.status, arg.type, arg.dbr);
}

EpicChannel::EpicChannel(char const* pv_name, ::capri priority)
: chan{nullptr}, monitor_id{nullptr}, is_conn{false}
{
	::ca_create_channel(pv_name, &EpicChannel::connection_callback, this, priority, &chan);
}

EpicChannel::~EpicChannel()
{
	if(chan) {
		std::cout << "Calling dtor for" << ::ca_name(chan) << '\n';
		StopMonitoring(); // No-op if not monitoring
		::ca_clear_channel(chan);
	}
}


EpicChannel::EpicChannel(EpicChannel && other) noexcept
{
	std::lock(other.io_mut, other.mon_mut);
	std::lock_guard<std::mutex> lk_io(other.io_mut, std::adopt_lock);
	std::lock_guard<std::mutex> lk_mon(other.mon_mut, std::adopt_lock);

	chan = std::exchange(other.chan, nullptr);
	monitor_id = std::exchange(other.monitor_id, nullptr);
	is_conn.store(other.is_conn.exchange(false, std::memory_order_relaxed),
					std::memory_order_relaxed);
}


::channel_state EpicChannel::CheckConnection() { return ::ca_state(chan); }
std::string_view EpicChannel::GetName() { return ::ca_name(chan); }


void EpicChannel::StartMonitoring()
{
	StartMonitoring(DBR_STRING, monitor_callback, nullptr);
}
void EpicChannel::StopMonitoring()
{
	// No-op if monitor_id is not set
	evid active_id{nullptr};
	{
		std::lock_guard lk(mon_mut);
		if(	monitor_id ) {
			active_id = monitor_id;
			monitor_id = nullptr;
		}
	}
	if(active_id) {
		int result = ca_clear_subscription(active_id);
		if(result != ECA_NORMAL) {
			std::cout << "Stop Monitoring Error: " << ::ca_message(result) << '\n';
		}
		::ca_flush_io();
	}
}

void EpicChannel::StartMonitoring(::chtype dbr_type, MonitorCallback func, void* pArg)
{
	// We do not support arrays at this time
	constexpr int elem_size = 1;
	{
		std::lock_guard lk(mon_mut);
		if(monitor_id == nullptr) {
			int result = ca_create_subscription(dbr_type, elem_size, chan, DBE_VALUE, func, pArg, &monitor_id);
			if(result != ECA_NORMAL) {
				std::cout << "Start Monitoring Error: " << ::ca_message(result) << '\n';
			}
		} else {
			std::cout << "Start Monitoring Error: Already monitoring this channel.\n";
		}
	}
	::ca_flush_io();
	return;
}
