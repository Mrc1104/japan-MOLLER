#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <cadef.h>
#include "EpicTypeTraits.h"


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//// AtomicEpicsType
template<typename T>
class AtomicEpicsType
{
	static_assert(std::is_trivially_copyable_v<T>,
		"Primary template must be trivially copyable.");
	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	std::atomic<T> data;
public:
	AtomicEpicsType(T const& input = T{});
	void Store(T const& input, std::memory_order mem_order = std::memory_order_seq_cst);
	T Load(std::memory_order mem_order = std::memory_order_seq_cst) const;
	T Exchange(T desired, std::memory_order mem_order = std::memory_order_seq_cst);
	bool CompareExchangeWeak(T& expected, T desired, std::memory_order success, std::memory_order failure);
	bool CompareExchangeWeak(T& expected, T desired, std::memory_order mem_order = std::memory_order_seq_cst);
};

template<typename T>
AtomicEpicsType<T>::AtomicEpicsType(T const& input) : data{input} {}


template<typename T>
void AtomicEpicsType<T>::Store(T const& val, std::memory_order mem_order) {
	data.store(val, mem_order);
}
template<typename T>
T AtomicEpicsType<T>::Load(std::memory_order mem_order) const {
	return data.load(mem_order);
}

template<typename T>
T AtomicEpicsType<T>::Exchange(T desired, std::memory_order mem_order)
{
	return data.exchange(desired, mem_order);
}

template<typename T>
bool AtomicEpicsType<T>::CompareExchangeWeak(T& expected, T desired, std::memory_order success, std::memory_order failure)
{
	return data.compare_exchange_weak(expected, desired, success, failure);
}

template<typename T>
bool AtomicEpicsType<T>::CompareExchangeWeak(T& expected, T desired, std::memory_order mem_order)
{
	return data.compare_exchange_weak(expected, desired, mem_order);
}

// Can I jsut make this accept a string_view?
template<>
class AtomicEpicsType<std::string>
{
	static constexpr std::size_t MAX_STR_LENGTH = 40;
	using EpicsType = typename EpicsTypeTraits<std::string>::epics_type;
	std::string data;
	mutable std::mutex  mut;

	void StoreImpl(std::string_view sv, std::memory_order mem_order);
public:
	AtomicEpicsType(std::string_view input = "", std::memory_order mem_order = std::memory_order_seq_cst);
	void Store(std::string const& input, std::memory_order mem_order = std::memory_order_seq_cst);
	void Store(EpicsType const& input, std::memory_order mem_order = std::memory_order_seq_cst);
	std::string Load(std::memory_order mem_order = std::memory_order_seq_cst) const;
	std::string Exchange(std::string desired, std::memory_order mem_order = std::memory_order_seq_cst);
	bool CompareExchangeWeak(std::string& expected, std::string desired, std::memory_order success, std::memory_order failure);
	bool CompareExchangeWeak(std::string& expected, std::string desired, std::memory_order mem_order = std::memory_order_seq_cst);
};


