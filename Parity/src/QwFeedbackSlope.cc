#include "QwFeedbackSlopes.h"
#include "ErrorHandling.h"
#include <cmath> // std::fclassify


QwIHWPSlope::QwIHWPSlope() : fSlopes{1.0, 1.0}, fIHWP() {}
double& QwIHWPSlope::operator[](IHWP state)       { return fSlopes[static_cast<int>(state)]; }
double  QwIHWPSlope::operator[](IHWP state) const { return fSlopes[static_cast<int>(state)]; }
double  QwIHWPSlope::GetSlope() const             { return this->operator[](fIHWP.GetState());}
void    QwIHWPSlope::AttachLogger(QwFeedbackLogger& logger) { fIHWP.AddObserver(&logger); }
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



double& QwPOSXYSlope::operator[](POS_TYPE pos)
{
	auto idx = static_cast<int>(pos);
	if(idx >= ARR_SIZE) {
		THROW_MSG("Given Index (" + std::to_string(idx) + ") >= Capacity (" +std::to_string(ARR_SIZE) ')');
	}
	return fSlopes_raw[idx];
}
double  QwPOSXYSlope::operator[](POS_TYPE pos) const
{
	auto idx = static_cast<int>(pos);
	if(idx >= ARR_SIZE) {
		THROW_MSG("Given Index (" + std::to_string(idx) + ") >= Capacity (" +std::to_string(ARR_SIZE) ')');
	}
	return fSlopes_raw[idx];
}

double  QwPOSXYSlope::GetSlope(POS_TYPE pos) const
{
	if(fIsCached == false) cache();
	return fSlopes_cached[static_cast<int>(pos)];
}

void QwPOSXYSlope::cache() const
{
	auto fyV = operator[](POS_TYPE::YV);
	auto fyU = operator[](POS_TYPE::YU);
	auto fxV = operator[](POS_TYPE::XV);
	auto fxU = operator[](POS_TYPE::XU);
	
	auto lin_comb_yv =  fyV / ( (fxU * fyV) - (fyU * fxV) );
    auto lin_comb_yu = -fyU / ( (fxU * fyV) - (fyU * fxV) );
    auto lin_comb_xv = -fxV / ( (fxU * fyV) - (fyU * fxV) );
    auto lin_comb_xu =  fxU / ( (fxU * fyV) - (fyU * fxV) );

	fSlopes_cached[static_cast<int>(POS_TYPE::YV)] = lin_comb_yv;
	fSlopes_cached[static_cast<int>(POS_TYPE::YU)] = lin_comb_yu;
	fSlopes_cached[static_cast<int>(POS_TYPE::XV)] = lin_comb_xv;
	fSlopes_cached[static_cast<int>(POS_TYPE::XU)] = lin_comb_xu;

	std::string log_payload = "PosXY Linear Combinations:\n"
	                        + "\tYV = " + std::to_string(lin_comb_yv)
	                        + "\tYU = " + std::to_string(lin_comb_yu)
	                        + "\tXV = " + std::to_string(lin_comb_xv)
	                        + "\tXU = " + std::to_string(lin_comb_xu) + '\n';
	Notify(QwFeedbackLogPayload{QwLogLevel::kMessage, std::move(log_payload)});
	fIsCached = true;
}

void QwPOSXYSlope::AttachLogger(QwFeedbackLogger& logger)
{
	AddObserver(logger);
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

