#include "EpicHandler.h"
#include "ErrorHandling.h"
#include <cadef.h>
#include <string_view>
std::string_view stringify(::channel_state const state)
{
	std::string_view sv;
	switch (state) {
		case cs_never_conn:
			sv = "STATE: IOC not found or unavailable.";
			break;
		case cs_prev_conn:
			sv = "STATE: IOC was found, but unavailable (previously connected to server).";
			break;
		case cs_conn:
			sv = "STATE: Valid chid, IOC was found, still available.";
			break;
		case cs_closed:
			sv = "STATE: Channel deleted by user.";
			break;
		default:
			sv = "STATE: Unknown IOC.";
			break;
	}
	return sv;
}
EpicHandler::EpicHandler()
{
    int result = ::ca_context_create(::ca_enable_preemptive_callback);
	if( result != ECA_NORMAL ) {
		THROW_ERROR("CA Error while opening channel: " + std::string(::ca_message(result)));
	}
}

EpicHandler::~EpicHandler()
{
	// I need to delete the channels first before closing the context
	channels.clear();
	::ca_flush_io();
	ca_context_destroy();
}


EpicChannel* EpicHandler::ConnectChannel(char const* pv_name, capri priority)
{
	auto new_ch = std::make_unique<EpicChannel>(pv_name, priority);
	auto ptr    = new_ch.get();
	channels.push_back( std::move(new_ch) );
	return ptr;
}

void EpicHandler::GetStatus(unsigned level)
{
	::ca_client_status(level);
}

void EpicHandler::CheckConnection(std::ostream& out)
{
	out << "Checking channel connections:\n";
	for( auto& ch : channels ) {
		auto name  = ch->GetName();
		auto state = ch->CheckConnection();
		out << name << " -- " << stringify(state) << '\n';
	}
}

