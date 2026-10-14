#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "VQwDataHandler.h" // Access EQwHandleType
#include "QwFeedbackLogger.h"
#include "QwFeedbackSlope.h"
#include "QwFeedbackSetpoints.h"
#include "QwFeedbackCorrectionPayload.h"

class VQwFeedbackImpl;
class QwFeedbackConfig;

struct Device
{
	Device(std::string name, std::string_view descr)
	: fName(std::move(name))
	, fFbType(GetFbCorrType(descr))
	, fQwType{VQwDataHandler::kHandleTypeUnknown}
	{
		if(fFbType == FbCorrType::ASYM) {
			fQwType = VQwDataHandler::kHandleTypeAsym;
		} else if(fFbType == FbCorrType::DIFFX || fFbType == FbCorrType::DIFFY) {
			fQwType = VQwDataHandler::kHandleTypeDiff;
		}
	}
	std::string   fName;
	FbCorrType    fFbType;
	VQwDataHandler::EQwHandleType fQwType;
};

class QwFeedback
{
	std::unique_ptr<VQwFeedbackImpl>   fPimpl;
	std::unique_ptr<VQwFeedbackSlopes> fSlope;
	QwFeedbackLogger fLogFile;
	std::vector<Device> fDevices;
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
	void ConfigureFeedback(QwFeedbackConfig const& config);
	void ApplyCorrection(QwFeedbackCorrectionPayload const& payload);
	std::vector<Device> const& RequestTargetDevices() const;
public:
	template<typename IndexType>
	void SetSlope(IndexType Index, double val);
};

template<typename IndexType>
void QwFeedback::SetSlope(IndexType index, double val)
{
	if(fSlope) fSlope->Set<IndexType>(index, val);
}

