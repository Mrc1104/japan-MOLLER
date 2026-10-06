#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <type_traits>
#include <db_access.h>
#include <cadef.h>


// Supported EPIC Types
template<typename T>
struct EpicsTypeTraits;

template<>
struct EpicsTypeTraits<double> {
	static constexpr ::chtype dbr_type = DBR_DOUBLE;
	using epics_type = dbr_double_t; // 64 bit
};
template<>
struct EpicsTypeTraits<int> {
	static constexpr ::chtype dbr_type = DBR_LONG;
	using epics_type = dbr_long_t; // 32 bit
};
template<>
struct EpicsTypeTraits<std::string> {
	static constexpr ::chtype dbr_type = DBR_STRING;
	using epics_type = dbr_string_t; // char[40]
};

/*
template<typename T>
class EpicsType
{
	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	T data;
public:
	EpicsType(T const& input = T{});
	template<typename U = EpicsType, typename = std::enable_if_t<!std::is_same_v<T,U>>>
	EpicsType(U const& input = U{});
	EpicsType& operator=(T const& input);

	operator T() const;	
	
};

template<typename T>
EpicsType::EpicsType(T const& input)
: data{input}
{ }
template<typename T>
template<typename U = EpicsType, typename = std::enable_if_t<!std::is_same_v<T,U>>>
EpicsType<T>::EpicsType(U const& input = U{})
: data(static_cast<T>(input))
{ }
template<typename T>
EpicsType<T>::operator T() const { return data; }
*/

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
	template<typename U = EpicsType, typename = std::enable_if_t<!std::is_same_v<T,U>>>
	void Store(U const& input, std::memory_order mem_order = std::memory_order_seq_cst);
	T Load(std::memory_order mem_order = std::memory_order_seq_cst) const;
};

template<typename T>
AtomicEpicsType<T>::AtomicEpicsType(T const& input) : data{input} {}

template<typename T>
template<typename U, typename>
void AtomicEpicsType<T>::Store(U const& dbr, std::memory_order mem_order)
{
	data.store(static_cast<T>( dbr ), mem_order);	
}

template<typename T>
void AtomicEpicsType<T>::Store(T const& val, std::memory_order mem_order) {
	data.store(val, mem_order);
}
template<typename T>
T AtomicEpicsType<T>::Load(std::memory_order mem_order) const {
	return data.load(mem_order);
}

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
};
