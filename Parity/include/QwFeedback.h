#pragma once
#include <string>
#include <string_view>
#include <functional>
#include <fstream>
#include <utility>
#include <variant>
#include "IHWP.h"
#include "VQwDataHandler.h" // Access EQwHandleType
#include "QwFeedbackLogger.h"

class QwFeedbackConfig
{
public:
	enum class TYPE	{ kTARGET, kIOC, kUNKNOWN };
private:
	TYPE type{TYPE::kUNKNOWN};
	std::string name;
	std::string descr;
private:
	std::string_view parse_pair_impl(std::string_view token, std::string_view target);
public:
	// Config Parsing
	bool parse_pair(std::string_view token);
	[[nodiscard]] bool isValid() const;
public:
	// Accesors
	TYPE getType() const;
	std::string const& getName() const& noexcept;
	std::string&& getName() && noexcept;
	std::string const& getDescr() const& noexcept;
	std::string&& getDescr() && noexcept;

public:
	// We all need friends
	friend std::ostream& operator<<(std::ostream& out, QwFeedbackConfig const& config);
};


class QwFeedbackSlope
{
	static_assert( static_cast<int>(IHWP::kIN) == 0
			&& static_cast<int>(IHWP::kOUT) == 1,
			"Expected: enum IHWP is used for indexing!\n");
	std::array<double, 2> fSlopes;
	IHWP_IOC fIHWP;
public:
	QwFeedbackSlope() : fSlopes{1.0, 1.0}, fIHWP() {}
	double& operator[](IHWP state)       { return fSlopes[static_cast<int>(state)]; }
	double  operator[](IHWP state) const { return fSlopes[static_cast<int>(state)]; }
	double  GetSlope() const             { return this->operator[](fIHWP.GetState());}
	void    AttachLogger(QwFeedbackLogger& logger) { fIHWP.AddObserver(&logger); }
};


// Templatize the EPICS Data Type
class QwFeedbackSetpoint : public EPICSObserver<double>, public Publisher<QwFeedbackLogPayload>
{
private:
	EpicChannel* fChannel;
	double fPrev;
	AtomicEpicsType<double> fCurr;
	std::string fName;
public:
	void Attach(const char* pv_name);
	void Update(double const& data) override;
	template<typename BinaryOp>
	std::future<void> ApplyCorrection(double corr) {
		double current = fCurr.Load(std::memory_order_acquire);
		while(!fCurr.CompareExchangeWeak(current, BinaryOp{}(current, corr), 
				std::memory_order_release, std::memory_order_acquire));
		return (fChannel) ? fChannel->PutAsync(DBR_DOUBLE, current) : std::future<void>{};
	}
public:
	QwFeedbackSetpoint();
	QwFeedbackSetpoint(EpicChannel* channel);
	~QwFeedbackSetpoint();
	QwFeedbackSetpoint(QwFeedbackSetpoint const& other);
	QwFeedbackSetpoint(QwFeedbackSetpoint&& other) = delete;
	QwFeedbackSetpoint& operator=(QwFeedbackSetpoint const& other) = delete;
	QwFeedbackSetpoint& operator=(QwFeedbackSetpoint&& other) = delete;
};

class VQwFeedbackImpl
{
public:
	virtual void ConfigureImpl(QwFeedbackConfig&& config)        = 0;
	virtual void ApplyCorrectionImpl(double const running_average) = 0;
	virtual std::pair<VQwDataHandler::EQwHandleType, std::string_view>
	RequestTargetDeviceImpl() const    = 0;
	virtual std::unique_ptr<VQwFeedbackImpl> Clone() const = 0;
};

class QwPITAFeedback : public VQwFeedbackImpl
{
	// Maybe use std::variant?
	using ADD = std::plus<double>;
	using SUB = std::minus<double>;
	static constexpr std::size_t fNumSetpointsExpected{8};
	std::array<QwFeedbackSetpoint, fNumSetpointsExpected> fPitaVoltages;
	std::size_t fNumSetpointsSet;
	std::string fDevice;
	QwFeedbackLogger& fLogger;
private:
	bool SetDeviceName(std::string&& name);
	bool AddSetpoint(std::string&& setp_name);
public:
	QwPITAFeedback(QwFeedbackLogger& logger);
	void ConfigureImpl(QwFeedbackConfig&& config) override;
	void ApplyCorrectionImpl(double const running_average) override;
	std::pair<VQwDataHandler::EQwHandleType, std::string_view>
	RequestTargetDeviceImpl() const override;
	std::unique_ptr<VQwFeedbackImpl> Clone() const override;
};

// Non-Virtual Interface
class QwFeedback
{
	// look into placement new
	std::unique_ptr<VQwFeedbackImpl> fPimpl;
	QwFeedbackSlope fSlope;
	QwFeedbackLogger fLogFile;
public:
	enum class TYPE {
		PITA,
		POSU,
		POSV//,etc
	};
private:
	void ConfigureFeedbackType(TYPE type);
public:
	QwFeedback();
	QwFeedback(QwFeedback const& other);
public:
	void ConfigureFeedbackType(std::string_view);
	bool ConfigureLogger(std::string const&, std::ios_base::openmode mode = std::ios::app);
public:
	void ConfigureFeedback(QwFeedbackConfig&& config);
	void ApplyCorrection(double const running_average);
	std::pair<VQwDataHandler::EQwHandleType, std::string_view>
	RequestTargetDevice() const;
public:
	void    SetSlope(IHWP state, double val);
	double  GetSlope(IHWP state) const;
	double& GetSlope(IHWP state);
	double  GetSlope() const;
};


