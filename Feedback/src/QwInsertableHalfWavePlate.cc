#include "QwInsertableHalfWavePlate.h"
#include <string>
#include <iostream>

#include "QwLog.h"

static char const *DEFAULT_IHWP_IOC = "IGL1I00DI24_24M";
	
IHWP::IHWP() : QwHalfWavePlate<int>(DEFAULT_IHWP_IOC)
{
	status = static_cast<int>( IHWP_STATUS::UNKNOWN );
}

IHWP::IHWP(std::string ioc) : QwHalfWavePlate<int>( std::move(ioc) )
{
	status = static_cast<int>( IHWP_STATUS::UNKNOWN );
}

void IHWP::Update()
{
	int tmp{};	
	hwp_ioc.Read(tmp);
	hwp_ioc.Sync();
	update_status(tmp);
}

IHWP_STATUS IHWP::GetIHWPStatus() const
{
	// Guaranteed to be within IHWP_STATUS
	// Range by 'void update_status()'
	return static_cast<IHWP_STATUS>( status );
}


bool IHWP::operator==(const IHWP_STATUS ihwp_status)
{
	return ( status == static_cast<int>(ihwp_status) );
}
bool IHWP::operator!=(const IHWP_STATUS ihwp_status)
{
	return !(*this == ihwp_status);
}

void IHWP::update_status(const int value)
{
	const IHWP_STATUS ihwp_value = static_cast<IHWP_STATUS>(value);
	switch(ihwp_value) {
	case IHWP_STATUS::OUT:
		status = value;
	break;
	case IHWP_STATUS::IN:
		status = value;
	break;
	default:
		status = static_cast<int>(IHWP_STATUS::UNKNOWN);
		QwError << "HWP Status is HWP_STATUS::UNKNOWN" << QwLog::endl;
	break;
	}
}

