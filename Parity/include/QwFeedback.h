#pragma once
#include <string>
#include <string_view>
#include <functional>
#include "VQwDataHandler.h" // Access EQwHandleType

class QwFeedbackConfig
{
	enum class TYPE	{ kTARGET, kIOC, kUNKNOWN };
	TYPE type{TYPE::kUNKNOWN};
	std::string name;
	std::string descr;
private:
	std::string_view parse_pair_impl(std::string_view token, std::string_view target);
public:
	bool parse_pair(std::string_view token);
	[[nodiscard]] bool isValid() const;
	TYPE getType() const;
	friend std::ostream& operator<<(std::ostream& out, QwFeedbackConfig const& config);
};

enum class IHWP {
	kIN = 0,
	kOUT
};

// TO DO:
// Make this connect to the IHWP IOC
class IHWP_IOC
{
	IHWP ihwp{IHWP::kIN};	
public:
	IHWP GetState() const { return ihwp; }
};

class Slope
{
	static_assert( static_cast<int>(IHWP::kIN) == 0
			&& static_cast<int>(IHWP::kOUT) == 1,
			"Expected: enum IHWP is used for indexing!\n");
	std::array<double, 2> fSlopes;
	IHWP_IOC fIOC;
public:
	Slope() : fSlopes{1.0, 1.0} {}
	double& operator[](IHWP state)       { return fSlopes[static_cast<int>(state)]; }
	double  operator[](IHWP state) const { return fSlopes[static_cast<int>(state)]; }
	double  GetSlope() const             { return this->operator[](fIOC.GetState());}
};

template<typename BinaryOp>
class FeedbackSetpoint {
	struct Setpoint
	{
		double fCurr;
		double fPrev;
	};
	Setpoint fSetpoint;
public:
	// Make this connect into the IOC
	FeedbackSetpoint() : fSetpoint{.fCurr{0.0}, .fPrev{0.0}} {}
	void Set(double val) {
		auto& [fCurr, fPrev] = fSetpoint;
		fPrev = fCurr;
		fCurr = val;
	}
	Setpoint ApplyCorrection(double corr) {
		auto& [fCurr, fPrev] = fSetpoint;
		fPrev = fCurr;
		fCurr = BinaryOp{}(corr, fPrev);
		return {fCurr, fPrev};
	}

};

class VQwFeedbackImpl
{
public:
	virtual void ConfigureImpl(QwFeedbackConfig const& config)        = 0;
	virtual void ApplyCorrectionImpl(double const running_average) = 0;
	virtual std::string_view const RequestTargetDeviceImpl() const    = 0;
	virtual std::unique_ptr<VQwFeedbackImpl> Clone() const = 0;
};
class QwPITAFeedback : public VQwFeedbackImpl
{
	// Maybe use std::variant?
	using ADD = std::plus<double>;
	using SUB = std::minus<double>;
	std::array<FeedbackSetpoint<ADD>, 4> fPitaVoltages1_4;
	std::array<FeedbackSetpoint<SUB>, 4> fPitaVoltages5_8;
	std::size_t NHVs;
public:
	void ConfigureImpl(QwFeedbackConfig const& config) override;
	void ApplyCorrectionImpl(double const running_average) override;
	std::string_view const RequestTargetDeviceImpl() const override {return std::string_view{};}
	std::unique_ptr<VQwFeedbackImpl> Clone() const override;
};

// Non-Virtual Interface
class QwFeedback
{
	// look into placement new
	std::unique_ptr<VQwFeedbackImpl> fPimpl;
	Slope fSlope;
public:
	enum class TYPE {
		PITA,
		POSU,
		POSV//,etc
	};
public:
	QwFeedback();
	QwFeedback(QwFeedback const& other);
public:
	void ConfigureFeedbackType(std::string_view);
	void ConfigureFeedbackType(TYPE type);
public:
	void Configure(QwFeedbackConfig const& config);
	void ApplyCorrection(double const running_average);
	std::string_view const RequestTargetDevice() const;
public:
	void    SetSlope(IHWP state, double val);
	double  GetSlope(IHWP state) const;
	double& GetSlope(IHWP state);
	double  GetSlope() const;
};

