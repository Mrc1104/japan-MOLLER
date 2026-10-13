#include "QwFeedbackCorrectionPayload.h"
#include <string>
#include "QwLog.h"
#include "ErrorHandling.h"

std::string_view stringify(FbCorrType type)
{
	std::string_view sv{};
	switch(type) {
	case FbCorrType::ASYM:
		sv = "ASYM";
		break;
	case FbCorrType::DIFFX:
		sv = "DIFFX";
		break;
	case FbCorrType::DIFFY:
		sv = "DIFFX";
		break;
	}
	return sv;
}

FbCorrType GetFbCorrType(std::string_view sv)
{
	FbCorrType type;
	if(sv.find("ASYM")) {
		type = FbCorrType::ASYM;
	} else if(sv.find("DIFFX")) {
		type = FbCorrType::DIFFX;
	} else if(sv.find("DIFFY")) {
		type = FbCorrType::DIFFY;
	} else {
		THROW_ERROR("UNKNOWN Feedback Correction Type!");
	}
	return type;
}

void QwFeedbackCorrectionPayload::SetImpl(int index, const double val)
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	fValues[index] = val;
}
double QwFeedbackCorrectionPayload::GetImpl(int index) const
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	return fValues[index];
}

std::unique_ptr<VQwFeedbackValues> QwFeedbackCorrectionPayload::Clone() const
{
	return std::make_unique<QwFeedbackCorrectionPayload>(*this);
}


void QwFeedbackDevice::SetImpl(int index, const ValueType val)
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	auto& str = fValues[index];
	if(!str.empty()) {
		QwWarning << "Feedback Device is already set for " << stringify(static_cast<FbCorrType>(index))
		          << "!\nIgnoring...\n";
		return;
	}
	str = std::string(val);
}
QwFeedbackDevice::ValueType QwFeedbackDevice::GetImpl(int index) const
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	return fValues[index];
}

std::unique_ptr<VQwEnumArray<std::string_view>> QwFeedbackDevice::Clone() const
{
	return std::make_unique<QwFeedbackDevice>(*this);
}


