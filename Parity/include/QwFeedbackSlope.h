#pragma once
#include <array>
#include "IHWP.h"
#include "Publisher.h"
#include "QwFeedbackLogger.h"

class VQwFeedbackSlope
{
public:
	virtual ~VQwFeedbackSlope() = default;
	virtual std::unique_ptr<VQwFeedbackSlope> Clone() const = 0;
	template<typename IndexType>
	void SetSlope(IndexType index, const double val);
	template<typename IndexType>
	double GetSlope(IndexType index = IndexType{}) const;
	double GetSlope(int index = 0) const;
private:
	virtual void    SetSlopeImpl(int index, const double val) = 0;
	virtual double  GetSlopeImpl(int index) const = 0;
};

template<typename IndexType>
void VQwFeedbackSlope::SetSlope(IndexType index, const double val)
{
	SetSlopeImpl(static_cast<int>(index), val);
}
template<typename IndexType>
double VQwFeedbackSlope::GetSlope(IndexType index) const
{
	return GetSlopeImpl(static_cast<int>(index));
}


class QwIHWPSlope : public VQwFeedbackSlope
{
	static_assert( static_cast<int>(IHWP::kIN) == 0
			&& static_cast<int>(IHWP::kOUT) == 1,
			"Expected: enum IHWP is used for indexing!\n");
	static constexpr std::size_t ARR_SIZE =  IHWP_IOC::fNumPossibleStates;
	std::array<double, ARR_SIZE> fSlopes;
	IHWP_IOC fIHWP;
	mutable bool fIsValid{false};

	void    SetSlopeImpl(int index, const double val) override;
	double  GetSlopeImpl(int index) const override;
	void check_isvalid() const;
public:
	QwIHWPSlope(QwFeedbackLogger& logger);
	std::unique_ptr<VQwFeedbackSlope> Clone() const override;
	bool    IsValid() const;
};

class QwPOSXYSlope : public VQwFeedbackSlope, public Publisher<QwFeedbackLogPayload>
{ 
public:
	enum class POS_TYPE
	{
		kYV = 0, kYU = 1,
		kXV = 2, kXU = 3
	};
	static_assert( static_cast<int>(POS_TYPE::kYV) == 0
			&& static_cast<int>(POS_TYPE::kXU) == 3,
			"Expected: enum POS_TYPE is used for indexing!\n");
	QwPOSXYSlope(QwFeedbackLogger& logger);
	std::unique_ptr<VQwFeedbackSlope> Clone() const override;
	void    AttachLogger(QwFeedbackLogger& logger);
	bool    IsValid() const;
private:
	void    SetSlopeImpl(int index, const double val) override;
	double  GetSlopeImpl(int index) const override;
	void cache() const;
	void check_isvalid() const;

	static constexpr std::size_t ARR_SIZE = 4;
	std::array<double, ARR_SIZE> fSlopes_raw{};
	mutable std::array<double, ARR_SIZE> fSlopes_cached{};
	mutable bool fIsCached{false};
	mutable bool fIsValid{false};
};
