#pragma once
#include <array>
#include "IHWP.h"
#include "Publisher.h"
#include "QwFeedbackLogger.h"

class QwIHWPSlope
{
	static_assert( static_cast<int>(IHWP::kIN) == 0
			&& static_cast<int>(IHWP::kOUT) == 1,
			"Expected: enum IHWP is used for indexing!\n");
	std::array<double, 2> fSlopes;
	IHWP_IOC fIHWP;
	mutable bool fIsValid{false};

	void check_isvalid() const;
public:
	QwIHWPSlope();
	double& operator[](IHWP state);
	double  operator[](IHWP state) const;
	double  GetSlope() const;
	void    AttachLogger(QwFeedbackLogger& logger);
	bool    IsValid() const;
};


class QwPOSXYSlope : public Publisher<QwFeedbackLogPayload>
{ 
public:
	enum class POS_TYPE
	{
		YV = 0, YU = 1,
		XV = 2, XU = 3
	};
	static_assert( static_cast<int>(POS_TYPE::YV) == 0
			&& static_cast<int>(POS_TYPE::XY) == 3,
			"Expected: enum POS_TYPE is used for indexing!\n");
	double& operator[](POS_TYPE pos);
	double  operator[](POS_TYPE pos) const;
	double  GetSlope(POS_TYPE pos) const;
	void    AttachLogger(QwFeedbackLogger& logger);
	bool    IsValid() const;
private:
	static constexpr ARR_SIZE = 4;
	std::array<double, ARR_SIZE> fSlopes_raw{};
	mutable std::array<double, ARR_SIZE> fSlopes_cached{};
	mutable bool fIsCached{false};
	mutable bool fIsValid{false};
	void cache() const;
	void check_isvalid() const;

};
