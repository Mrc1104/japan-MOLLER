#pragma once
#include <string_view>
#include <atomic>
#include <mutex>
#include <memory>
#include <future>
#include <type_traits>
#include <utility>
#include <cadef.h>
#include <iostream>
#include <stdexcept>
#include <optional>
#include "EpicTypes.h"
#include "ChannelObserver.h"
#include "Publisher.h"
#include "QwLog.h"

class EpicChannel
{
	// chid is an opaque ptr
	// think void*
	chid chan;
	std::atomic<bool> is_conn;
	std::mutex io_mut;
	static void connection_callback(::connection_handler_args arg) noexcept;
	static void monitor_callback(::event_handler_args arg) noexcept;
	template<typename T>
	static void monitor_value_callback(event_handler_args arg) noexcept;
	static void async_get_callback(::event_handler_args arg);
	static void async_put_callback(::event_handler_args arg);
public:
	EpicChannel(char const* pv_name, ::capri priority);
	~EpicChannel();
	EpicChannel(EpicChannel const&)            = delete;
	EpicChannel& operator=(EpicChannel const&) = delete;
	EpicChannel(EpicChannel && other) noexcept;
	EpicChannel& operator=(EpicChannel && other) = delete;
public:
	std::string_view GetName();
	::channel_state CheckConnection();
	using MonitorCallback = void(*)(::event_handler_args);
	[[nodiscard]]
	::evid StartMonitoring(::chtype dbr_type, MonitorCallback func, void* pArg) noexcept;
	[[nodiscard]]
	::evid StartMonitoring() noexcept;
	void StopMonitoring(::evid monitor_id) noexcept;
	template <typename T>
	void StartMonitoring(Observer<T>* observer) noexcept;
	template <typename T>
	void StopMonitoring(Observer<T>* observer) noexcept;
public:
	template<typename T>
	std::future<T> GetAsync(::chtype dbr_type);
	template<typename T>
	std::future<void> PutAsync(::chtype dbr_type, T&& value);

	class AsyncContextBase {
	public:
		virtual ~AsyncContextBase() = default;
		virtual void Fulfill(int status, chtype dbr_type, const void* dbr_ptr) = 0;
		virtual void Start(){};
		virtual void Stop(){};
	};
	template<typename T>
	class GetRequestContext : public AsyncContextBase
	{
		std::promise<T> promise;
	public:
		void Fulfill(int status, chtype dbr_type, const void* dbr_ptr) override;
		void SetException(std::exception_ptr except);
		std::future<T> GetFuture();
	};
	template<typename T>
	class PutRequestContext : public AsyncContextBase
	{
		std::promise<void> promise;
		T data;
	public:
		template<typename ...Args>
		PutRequestContext(Args&&... args);
		void Fulfill(int status, chtype dbr_type, const void* dbr_ptr) override;
		void SetException(std::exception_ptr except);
		std::future<void> GetFuture();
		void const* GetValuePtr() const noexcept;

	};
	template<typename T>
	class PublisherContext : public AsyncContextBase, public Publisher<T>
	{
		// Publisher contains all the 'Subject' logic
		// Maybe this also should hold onto the ::evid monitor_id too
		EpicChannel* fChannel;
		std::mutex ctx_mut;
		::evid monitor_id;
	public:
		PublisherContext(EpicChannel* chan);
		~PublisherContext();
		PublisherContext(PublisherContext const& other) = delete;
		PublisherContext& operator=(PublisherContext const& other) = delete;
		PublisherContext(PublisherContext&& other) = delete;
		PublisherContext& operator=(PublisherContext&& other) = delete;
		// Loops over all of the Subscribed values and updates them
		void Fulfill(int status, chtype dbr_type, const void* dbr_ptr) override;
		// No Op if already monitoring
		void Start() override;
		void Stop() override;
	};
	QwThreadSafeMap<::chtype, AsyncContextBase*> th_map;

};
template <typename T>
void EpicChannel::monitor_value_callback(event_handler_args arg) noexcept
{
	auto* context( static_cast<PublisherContext<T>*>(arg.usr));
	if(!context) return;
	context->Fulfill(arg.status, arg.type, arg.dbr);
}



template<typename T>
std::future<T> EpicChannel::GetAsync(::chtype dbr_type)
{
	auto context = std::make_unique<GetRequestContext<T>>();
	auto future  = context->GetFuture();
	try {
		if(!is_conn.load()) {
			throw std::runtime_error("Async Read Error: Channel is disconnected.");
		}
		int result;
		{	
			std::lock_guard<std::mutex> lk(io_mut);
			result = ::ca_get_callback(dbr_type, chan, &EpicChannel::async_get_callback, context.get());
		}
		if(result != ECA_NORMAL) {
			throw std::runtime_error("Async Read Error: " + std::string(::ca_message(result)));
		}
		context.release();
		::ca_flush_io();
	} catch(...) {
		context->SetException(std::current_exception());
	}
	return future;
}

template<typename T>
std::future<void> EpicChannel::PutAsync(::chtype dbr_type, T&& value)
{
	// how to avoid all heap allocs?
	auto context = std::make_unique<PutRequestContext<std::decay_t<T>>>(std::forward<T>(value));
	auto future  = context->GetFuture();
	try {
		if(!is_conn.load()) {
			throw std::runtime_error("Async Write Error: Channel is disconnected.");
		}
		int result;
		{	
			std::lock_guard<std::mutex> lk(io_mut);
			result = ::ca_put_callback(dbr_type, chan, context->GetValuePtr(), &EpicChannel::async_put_callback, context.get());
		}
		if(result != ECA_NORMAL) {
			throw std::runtime_error("Async Write Error: " + std::string(::ca_message(result)));
		}
		context.release();
		::ca_flush_io();
	} catch(...) {
		context->SetException(std::current_exception());
	}
	return future;
}

template<typename T>
std::future<T> EpicChannel::GetRequestContext<T>::GetFuture() { return promise.get_future(); }

template<typename T>
void EpicChannel::GetRequestContext<T>::SetException(std::exception_ptr except) { promise.set_exception(except); }

template<typename T>
void EpicChannel::GetRequestContext<T>::Fulfill(int status, chtype dbr_type, const void* dbr_ptr)
{
	try {
		if(status != ECA_NORMAL) {
			throw std::runtime_error("Network Read Error: " + std::string(::ca_message(status)));
		}
		if(!dbr_ptr) {
			throw std::runtime_error("Network read error: payload is empty");
		}
		T const* data = static_cast<T const*>( dbr_ptr );
		promise.set_value(*data);
	} catch(...) {
		// Return the exception to the caller
		promise.set_exception(std::current_exception());
	}
}

template<typename T> template<typename ...Args>
EpicChannel::PutRequestContext<T>::PutRequestContext(Args&&... args)
: data(std::forward<Args>(args)...)
{ }
template<typename T>
std::future<void> EpicChannel::PutRequestContext<T>::GetFuture()
{
	return promise.get_future();
}
template<typename T>
void const* EpicChannel::PutRequestContext<T>::GetValuePtr() const noexcept
{
	return &data;
}
template<typename T>
void EpicChannel::PutRequestContext<T>::Fulfill(int status, chtype dbr_type, const void* dbr_ptr)
{
	try {
		if(status != ECA_NORMAL) {
			throw std::runtime_error("Network Write Error: " + std::string(::ca_message(status)));
		}
		promise.set_value();
	} catch(...) {
		promise.set_exception(std::current_exception());
	}
}
template<typename T>
void EpicChannel::PutRequestContext<T>::SetException(std::exception_ptr except)
{
	promise.set_exception(except);
}

template <typename T>
void EpicChannel::StartMonitoring(Observer<T>* observer) noexcept
{
	// I want to have a map with ::chtype as the key
	constexpr auto key = EpicsTypeTraits<T>::dbr_type;
	if(!th_map.Find(key)) {
		th_map.InsertOrAssign(key, new PublisherContext<T>(this));
	}
	auto publisher_context = static_cast<PublisherContext<T>*>(*th_map.Get(key));
	publisher_context->AddObserver(observer);
	publisher_context->Start();
}
template <typename T>
void EpicChannel::StopMonitoring(Observer<T>* observer) noexcept
{
	// I want to have a map with ::chtype as the key
	constexpr auto key = EpicsTypeTraits<T>::dbr_type;
	if(th_map.Find(key)) {
		auto publisher_context = static_cast<PublisherContext<T>*>(*th_map.Get(key));
		publisher_context->RemoveObserver(observer);
		publisher_context->Stop();
	}
}
template <typename T>
EpicChannel::PublisherContext<T>::PublisherContext(EpicChannel* chan)
: fChannel(chan)
, monitor_id{nullptr}
{ }
template <typename T>
EpicChannel::PublisherContext<T>::~PublisherContext()
{
	::evid active_id{nullptr};
	{
		std::lock_guard<std::mutex> lk(ctx_mut);
		active_id = monitor_id;
		monitor_id = nullptr;
	}
	fChannel->StopMonitoring(active_id);
}

template <typename T>
void EpicChannel::PublisherContext<T>::Fulfill(int status, chtype dbr_type, const void* dbr_ptr)
{
	if(status == ECA_NORMAL && dbr_ptr ) {
		T const* data = static_cast<T const*>( dbr_ptr );
		this->Notify(*data);
	}
}

template <typename T>
void EpicChannel::PublisherContext<T>::Start()
{
	constexpr ::chtype dbr_type = EpicsTypeTraits<T>::dbr_type;
	if(!fChannel->is_conn.load()) { return; }
	std::lock_guard<std::mutex> lk(ctx_mut);
	if(monitor_id != nullptr) { return; }
	monitor_id = fChannel->StartMonitoring(EpicsTypeTraits<T>::dbr_type, monitor_value_callback<T>, this);
}

template <typename T>
void EpicChannel::PublisherContext<T>::Stop()
{
	std::lock_guard<std::mutex> lk(ctx_mut);
	if(this->GetObserverCount() > 0 ) return;
	if( monitor_id )  {
		fChannel->StopMonitoring(monitor_id);
		monitor_id = nullptr;
	}
}

