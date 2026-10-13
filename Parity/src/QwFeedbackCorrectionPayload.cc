#include "QwFeedbackCorrectionPayload.h"

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

