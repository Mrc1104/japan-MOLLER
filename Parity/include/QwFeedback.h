#pragma once
#include <string>
#include <string_view>
#include <utility>
#include "VQwDataHandler.h" // Access EQwHandleType
#include "QwFeedbackLogger.h"
#include "QwFeedbackSlope.h"
#include "QwFeedbackSetpoints.h"

class VQwFeedbackImpl;
class QwFeedbackConfig;

class QwFeedback
{
	// look into placement new
	std::unique_ptr<VQwFeedbackImpl> fPimpl;
	std::unique_ptr<VQwEnumArray>    fSlope;
	QwFeedbackLogger fLogFile;
public:
	enum class TYPE {
		PITA,
		APOSU,
		APOSV,
		IA,
		POSXY,
		UNKNOWN
	};
private:
	void ConfigureFeedbackType(TYPE type);
public:
	QwFeedback();
	QwFeedback(QwFeedback const& other);
public:
	TYPE GetTypeOfFeedback(std::string_view sv) const;
	TYPE ConfigureFeedbackType(std::string_view);
	bool ConfigureLogger(std::string const&, std::ios_base::openmode mode = std::ios::app);
public:
	void ConfigureFeedback(QwFeedbackConfig&& config);
	void ApplyCorrection(VQwHardwareChannel const* device);
	std::pair<VQwDataHandler::EQwHandleType, std::string_view>
	RequestTargetDevice() const;
public:
	template<typename IndexType>
	void SetSlope(IndexType Index, double val);
};

template<typename IndexType>
void QwFeedback::SetSlope(IndexType index, double val)
{
	if(fSlope) fSlope->Set<IndexType>(index, val);
}

