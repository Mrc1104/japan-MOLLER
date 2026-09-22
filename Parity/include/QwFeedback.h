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
class Slope
{
	static_assert( static_cast<int>(IHWP::kIN) == 0
			&& static_cast<int>(IHWP::kOUT) == 1,
			"Expected: enum IHWP is used for indexing!\n");
	std::array<double, 2> slopes;
	public:
	Slope() : slopes{1.0, 1.0} {}
	double& operator[](IHWP state)       { return slopes[static_cast<int>(state)]; }
	double  operator[](IHWP state) const { return slopes[static_cast<int>(state)]; }
};

template<typename BinaryOp>
class FeedbackSetpoint {
	double fCurr;
	double fPrev;
public:
	FeedbackSetpoint() : fCurr{0.0}, fPrev{0.0} {}
	void Set(double val) { fCurr = val; }
	double ApplyCorrection(double corr) {
		fPrev = fCurr;
		fCurr = BinaryOp{}(corr, fPrev);
	}

};

class VQwFeedbackImpl
{
public:
	virtual void ConfigureImpl(QwFeedbackConfig const& config)        = 0;
	virtual void CalculateCorectionImpl(double const running_average) = 0;
	virtual std::string_view const RequestTargetDeviceImpl() const    = 0;
	virtual std::unique_ptr<VQwFeedbackImpl> Clone() const = 0;
};
class QwPITAFeedback : public VQwFeedbackImpl
{
	using ADD = std::plus<double>;
	using SUB = std::minus<double>;
	FeedbackSetpoint<ADD> setpoint1;
	FeedbackSetpoint<ADD> setpoint2;
	FeedbackSetpoint<ADD> setpoint3;
	FeedbackSetpoint<ADD> setpoint4;
	FeedbackSetpoint<SUB> setpoint5;
	FeedbackSetpoint<SUB> setpoint6;
	FeedbackSetpoint<SUB> setpoint7;
	FeedbackSetpoint<SUB> setpoint8;

public:
	void ConfigureImpl(QwFeedbackConfig const& config) override {}
	void CalculateCorectionImpl(double const running_average) override {}
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
	void CalculateCorection(double const running_average);
	std::string_view const RequestTargetDevice() const;
public:
	void    SetSlope(IHWP state, double val);
	double  GetSlope(IHWP state) const;
	double& GetSlope(IHWP state);
};

