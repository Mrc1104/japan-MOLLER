#pragma once
#include <array>
#include "VQwEnumArray.h"
#include "QwIHWP.h"
#include "QwPublisher.h"
#include "QwFeedbackLogger.h"

class QwIHWPSlope : public VQwFeedbackSlopes
{
	static_assert( static_cast<int>(IHWP::kIN) == 0
			&& static_cast<int>(IHWP::kOUT) == 1,
			"Expected: enum IHWP is used for indexing!\n");
	static constexpr std::size_t ARR_SIZE =  IHWP_IOC::fNumPossibleStates;
	std::array<double, ARR_SIZE> fSlopes;
	IHWP_IOC fIHWP;
	mutable bool fIsValid{false};

	void    SetImpl(int index, const double val) override;
	double  GetImpl(int index) const override;
	void    check_isvalid() const;
public:
	QwIHWPSlope(QwFeedbackLogger& logger);
	std::unique_ptr<VQwFeedbackSlopes> Clone() const override;
	bool IsValid() const;
};

class QwPOSXYSlope : public VQwFeedbackSlopes, public Publisher<QwFeedbackLogPayload>
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
	std::unique_ptr<VQwFeedbackSlopes> Clone() const override;
	void    AttachLogger(QwFeedbackLogger& logger);
	bool    IsValid() const;
private:
	void    SetImpl(int index, const double val) override;
	double  GetImpl(int index) const override;
	void    cache() const;
	void    check_isvalid() const;

	static constexpr std::size_t ARR_SIZE = 4;
	std::array<double, ARR_SIZE> fSlopes_raw{};
	mutable std::array<double, ARR_SIZE> fSlopes_cached{};
	mutable bool fIsCached{false};
	mutable bool fIsValid{false};
};
