#pragma once
#include "EpicTypes.h"
#include "EpicChannel.h"
#include <string_view>

template<typename T>
class Observer
{
public:
	virtual ~Observer() = default;
	virtual void Update(T const& data) = 0;
};

template<typename T>
class ChannelObserver : public Observer<T>
{
	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	AtomicEpicsType<T> fValue;
public:
	ChannelObserver();
	void Update(EpicsType const& data) override;
	T GetValue() const;
};

template<typename T>
ChannelObserver<T>::ChannelObserver()
: fValue{}
{ }

template<typename T>
void ChannelObserver<T>::Update(EpicsType const& data)
{
	fValue.Store(data);	
	std::cout << "Storing value: " << data << '\n';
	// Do other things
}

template<typename T>
T ChannelObserver<T>::GetValue() const
{
	return fValue.Load();
}
