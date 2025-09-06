#include "QwHalfWavePlate.h"

template<typename T>
QwHalfWavePlate<T>::QwHalfWavePlate(std::string ioc) : hwp_ioc(std::move(ioc)), status{} { }

// template<typename T>
// QwHalfWavePlate<T>::~QwHalfwavePlate() { }

template<typename T>
void QwHalfWavePlate<T>::Update()
{
	hwp_ioc.read(status);
}

template<typename T>
T QwHalfWavePlate<T>::GetStatus() const
{
	return status;
}

// QwEPICS does not support any arbitrary types so we are limited to
// the types it supports. To enforce this, we will explicitly instantiate
// the template definitions here.
// Note: To relax requiring the explicit defintions, move the impl. into QwHalfWavePlate.h
template class QwHalfWavePlate<int>;   // Insertable   HWP is either 'In' or 'Out' ( discrete )
template class QwHalfWavePlate<double>;// The Rotating HWP goes from 0-360         (continuous)
