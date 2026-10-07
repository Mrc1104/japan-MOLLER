#pragma once
#include <db_access.h>
#include <type_traits>

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


template<typename T, typename = void>
struct is_epic_supported : std::false_type{};

template<typename T>
struct is_epic_supported<T, std::void_t<typename EpicsTypeTraits<T>::epics_type>> : std::true_type{};
