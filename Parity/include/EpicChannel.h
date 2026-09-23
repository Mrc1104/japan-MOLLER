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

struct EpicString {
	dbr_string_t data;
};


class EpicChannel
{
	// chid is an opaque ptr
	// think void*
	chid chan;
	evid monitor_id;
	std::atomic<bool> is_conn;
	std::mutex io_mut;
	std::mutex mon_mut;
	static void connection_callback(::connection_handler_args arg) noexcept;
	static void monitor_callback(::event_handler_args arg) noexcept;
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
	void StartMonitoring(::chtype dbr_type, MonitorCallback func, void* pArg);
	void StartMonitoring();
	void StopMonitoring();
public:
	template<typename T>
	std::future<T> GetAsync(::chtype dbr_type);
	template<typename T>
	std::future<void> PutAsync(::chtype dbr_type, T&& value);

	class AsyncContextBase {
	public:
		virtual ~AsyncContextBase() = default;
		virtual void Fulfill(int status, chtype dbr_type, const void* dbr_ptr) = 0;
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
};


template<typename T>
std::future<T> EpicChannel::GetAsync(::chtype dbr_type)
{
	// how to avoid all heap allocs?
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

template<typename T>
template<typename ...Args>
EpicChannel::PutRequestContext<T>::PutRequestContext(Args&&... args)
: data(std::forward<Args>(args)...) { }
template<typename T>
std::future<void> EpicChannel::PutRequestContext<T>::GetFuture() { return promise.get_future(); }
template<typename T>
void const* EpicChannel::PutRequestContext<T>::GetValuePtr() const noexcept { return &data; }
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
void EpicChannel::PutRequestContext<T>::SetException(std::exception_ptr except) { promise.set_exception(except); }
