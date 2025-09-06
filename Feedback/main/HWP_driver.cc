#include "QwEPICSControl.h"
#include "QwHalfWavePlate.h"
#include "QwInsertableHalfWavePlate.h"
#include "QwRotatableHalfWavePlate.h"
#include <iostream>
#include <string>

int main()
{
	IHWP ihwp("IGL1I00DI24_24M");	
	for(int i = 0; i < 5; i++) {
		std::cout << "Insertable Wave Plate" << std::endl;
		ihwp.Update();
		if(ihwp == IHWP_STATUS::UNKNOWN) {
			std::cout << "UNKNOWN\n";
			std::cout << ihwp.GetStatus() << std::endl;
		}
		if(ihwp == IHWP_STATUS::IN) {
			std::cout << "IN\n";
			std::cout << ihwp.GetStatus() << std::endl;
		}
		if(ihwp == IHWP_STATUS::OUT) {
			std::cout << "OUT\n";
			std::cout << ihwp.GetStatus() << std::endl;
		}
		std::cout << "-----------------------------" << std::endl;
	}

	RHWP rhwp("IGL1I00DI24_24M");	
	for(int i = 0; i < 5; i++) {
		std::cout << "Rotatable Wave Plate" << std::endl;
		rhwp.Update();
		std::cout << "RAW: " << rhwp.GetStatus() << std::endl;
		std::cout << "ARB: " << rhwp.GetStatus_Arb() << std::endl;
		std::cout << "DEG: " << rhwp.GetStatus_Deg() << std::endl;
		std::cout << "RAD: " << rhwp.GetStatus_Rad() << std::endl;
		std::cout << "-----------------------------" << std::endl;
	}

	return 0;
}
