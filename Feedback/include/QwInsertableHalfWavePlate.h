#ifndef __QW_INSERTABLE_HALF_WAVE_PLATE__
#define __QW_INSERTABLE_HALF_WAVE_PLATE__
#include "QwHalfWavePlate.h"

#include <string>

enum class IHWP_STATUS
{
	OUT = 0,
	IN,
	UNKNOWN
};

class IHWP : public QwHalfWavePlate<int> // 'In' or 'Out' (Discrete)
{
public:
	IHWP();
	IHWP(std::string ioc);

	void Update()         override;
	// It would be nice if I could return
	// a IHWP_STATUS
	// int GetStatus() const override;
	IHWP_STATUS GetIHWPStatus() const;

	bool operator==(const IHWP_STATUS ihwp_status);
	bool operator!=(const IHWP_STATUS ihwp_status);
private:
	void update_status(const int value);
};

#endif
