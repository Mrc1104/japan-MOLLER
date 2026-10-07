#pragma once
#include <vector>
#include <mutex>

#include "EpicTypeTraits.h"
#include "ChannelObserver.h"

// Forward Declaration
template<typename T>
class Observer;

template<typename T>
class Publisher
{
	mutable std::recursive_mutex mut;
	std::vector<Observer<T>*> Observers;

protected:
	void Notify(T const& data);
public:
	virtual ~Publisher() = default;
	void AddObserver(Observer<T>* observer);
	bool RemoveObserver(Observer<T>* observer);
	std::size_t GetObserverCount() const;
};

template<typename T>
class EPICSPublisher : public Publisher<T>
{
	static_assert(is_epic_supported<T>::value,
				"EPICSPublisher is limited only to EPICS Supported Types");

	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	using MonitorCallback = void(*)(event_handler_args);
};


template<typename T>
void Publisher<T>::AddObserver(Observer<T>* observer)
{
	std::lock_guard<std::recursive_mutex> lk(mut);
	Observers.push_back(observer);
}

template<typename T>
bool Publisher<T>::RemoveObserver(Observer<T>* observer)
{
	using std::begin, std::end;
	std::lock_guard<std::recursive_mutex> lk(mut);
	for( auto it = end(Observers); it != begin(Observers); ) {
		it--;
		if( *it == observer ) {
			Observers.erase(it);
			return true;
		}
	}
	return false;
}

template<typename T>
void Publisher<T>::Notify(T const& data)
{
	std::lock_guard<std::recursive_mutex> lk(mut);
	for( auto observer : Observers ) {
		observer->Update(data);
	}
}
template<typename T>
std::size_t Publisher<T>::GetObserverCount() const
{
	std::lock_guard<std::recursive_mutex> lk(mut);
	return Observers.size();

}

