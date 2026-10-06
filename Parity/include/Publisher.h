#pragma once
#include <vector>
#include <mutex>

#include "EpicTypes.h"
#include "EpicChannel.h"
#include "ChannelObserver.h"

template<typename T>
class Publisher
{
	mutable std::recursive_mutex mut;
	std::vector<Observer<T>*> Observers;

protected:
	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	using MonitorCallback = void(*)(event_handler_args);
	void Notify(EpicsType const& data);
public:
	virtual ~Publisher() = default;
	void AddObserver(Observer<T>* observer);
	bool RemoveObserver(Observer<T>* observer);
	std::size_t GetObserverCount() const;
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
void Publisher<T>::Notify(EpicsType const& data)
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

