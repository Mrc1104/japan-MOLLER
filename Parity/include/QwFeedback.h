#pragma once
#include <string>
#include <string_view>
#include <utility>
#include "VQwDataHandler.h" // Access EQwHandleType
#include "QwFeedbackLogger.h"
#include "QwFeedbackSetpoints.h"

class VQwFeedbackImpl;
class QwFeedbackConfig;


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


