#include "QwFeedbackSlope.h"
#include "cassert"
#include <cmath> // std::fclassify

double VQwEnumArray::Get(int index) const
{
	return GetImpl(index);
}


QwIHWPSlope::QwIHWPSlope(QwFeedbackLogger& logger) : fSlopes{1.0, 1.0}, fIHWP() 
{
	fIHWP.AddObserver(&logger);
}

std::unique_ptr<VQwEnumArray> QwIHWPSlope::Clone() const
{
	return std::make_unique<QwIHWPSlope>( *this );
}
void QwIHWPSlope::SetImpl(int index, const double val)
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	fSlopes[index] = val;
}
double QwIHWPSlope::GetImpl(int index) const
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	if(IsValid() == false) {
		QwError << "Invalid Slopes Given! Setting slope = 1.0\n";
		return 1.0;
	}
	return fSlopes[static_cast<int>(fIHWP.GetState())];
}
bool QwIHWPSlope::IsValid() const
{
	if(fIsValid == false) check_isvalid();
	return fIsValid;
}
void QwIHWPSlope::check_isvalid() const
{
	bool is_valid = true;
	for( auto& elem : fSlopes ) {
		if(std::fpclassify(elem) != FP_NORMAL) is_valid = false;
	}
	fIsValid = is_valid;
}

QwPOSXYSlope::QwPOSXYSlope(QwFeedbackLogger& logger)
{
	AddObserver(&logger);
}

std::unique_ptr<VQwEnumArray> QwPOSXYSlope::Clone() const
{
	return std::make_unique<QwPOSXYSlope>( *this );
}

void QwPOSXYSlope::SetImpl(int index, const double val) 
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	fSlopes_raw[index] = val;
}
double QwPOSXYSlope::GetImpl(int index) const
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	if(IsValid() == false) {
		QwError << "Invalid Slopes Given! Setting slope = 1.0\n";
		return 1.0;
	}
	if(fIsCached == false) cache();
	return fSlopes_cached[index];
}

void QwPOSXYSlope::cache() const
{
	using namespace std::string_literals;
	auto fyV = fSlopes_raw[static_cast<int>(POS_TYPE::kYV)];
	auto fyU = fSlopes_raw[static_cast<int>(POS_TYPE::kYU)];
	auto fxV = fSlopes_raw[static_cast<int>(POS_TYPE::kXV)];
	auto fxU = fSlopes_raw[static_cast<int>(POS_TYPE::kXU)];
	
	auto lin_comb_yv =  fyV / ( (fxU * fyV) - (fyU * fxV) );
    auto lin_comb_yu = -fyU / ( (fxU * fyV) - (fyU * fxV) );
    auto lin_comb_xv = -fxV / ( (fxU * fyV) - (fyU * fxV) );
    auto lin_comb_xu =  fxU / ( (fxU * fyV) - (fyU * fxV) );

	fSlopes_cached[static_cast<int>(POS_TYPE::kYV)] = lin_comb_yv;
	fSlopes_cached[static_cast<int>(POS_TYPE::kYU)] = lin_comb_yu;
	fSlopes_cached[static_cast<int>(POS_TYPE::kXV)] = lin_comb_xv;
	fSlopes_cached[static_cast<int>(POS_TYPE::kXU)] = lin_comb_xu;

	std::string log_payload = "PosXY Linear Combinations:\n"s
	                        + "\tYV = " + std::to_string(lin_comb_yv)
	                        + "\tYU = " + std::to_string(lin_comb_yu)
	                        + "\tXV = " + std::to_string(lin_comb_xv)
	                        + "\tXU = " + std::to_string(lin_comb_xu) + '\n';
	Notify(QwFeedbackLogPayload{QwLogLevel::kMessage, std::move(log_payload)});
	fIsCached = true;
}

void QwPOSXYSlope::AttachLogger(QwFeedbackLogger& logger)
{
	AddObserver(&logger);
}

bool QwPOSXYSlope::IsValid() const
{
	if(fIsValid == false) check_isvalid();
	return fIsValid;
}
void QwPOSXYSlope::check_isvalid() const
{
	bool is_valid = true;
	for( auto& elem : fSlopes_raw ) {
		if(std::fpclassify(elem) != FP_NORMAL) is_valid = false;
	}
	fIsValid = is_valid;
}
