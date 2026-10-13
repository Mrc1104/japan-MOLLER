#pragma once
#include <memory>

template<typename T = double>
class VQwEnumArray
{
public:
	virtual ~VQwEnumArray() = default;
	virtual std::unique_ptr<VQwEnumArray> Clone() const = 0;
	template<typename IndexType>
	void Set(IndexType index, const T val);
	template<typename IndexType>
	T Get(IndexType index = IndexType{}) const;
	T Get(int index = 0) const;
private:
	virtual void    SetImpl(int index, const T val) = 0;
	virtual T  GetImpl(int index) const = 0;
};

template<typename T>
T VQwEnumArray<T>::Get(int index) const
{
	return GetImpl(index);
}


template<typename T>
template<typename IndexType>
void VQwEnumArray<T>::Set(IndexType index, const T val)
{
	SetImpl(static_cast<int>(index), val);
}

template<typename T>
template<typename IndexType>
T VQwEnumArray<T>::Get(IndexType index) const
{
	return GetImpl(static_cast<int>(index));
}

using VQwFeedbackSlopes = VQwEnumArray<double>;
using VQwFeedbackValues = VQwEnumArray<double>;
