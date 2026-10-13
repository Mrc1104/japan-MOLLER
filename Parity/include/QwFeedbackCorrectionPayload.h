#pragma once
#include <array>
#include <cassert>
#include <memory>
#include <string_view>
#include "VQwEnumArray.h"

enum class FbCorrType
{
	ASYM,
	DIFFX,
	DIFFY
};
std::string_view stringify(FbCorrType type);
FbCorrType GetFbCorrType(std::string_view);



class QwFeedbackCorrectionPayload : public VQwFeedbackValues
{
	static_assert( static_cast<int>(FbCorrType::ASYM) == 0
			&& static_cast<int>(FbCorrType::DIFFY) == 2,
			"Expected: enum POS_TYPE is used for indexing!\n");

	static constexpr std::size_t ARR_SIZE = 3;
	std::array<double, ARR_SIZE> fValues{};

	std::unique_ptr<VQwFeedbackValues> Clone() const override;
public:
	void    SetImpl(int index, const double val) override;
	double  GetImpl(int index) const override;
};

class QwFeedbackDevice : public VQwEnumArray<std::string_view>
{
	static_assert( static_cast<int>(FbCorrType::ASYM) == 0
			&& static_cast<int>(FbCorrType::DIFFY) == 2,
			"Expected: enum POS_TYPE is used for indexing!\n");
	using ValueType = VQwEnumArray<std::string_view>::ValueType;

	static constexpr std::size_t ARR_SIZE = 3;
	std::array<std::string, ARR_SIZE> fValues{};
	void    SetImpl(int index, const ValueType val) override;
	ValueType  GetImpl(int index) const override;
public:
	std::unique_ptr<VQwEnumArray<std::string_view>> Clone() const override;
};
