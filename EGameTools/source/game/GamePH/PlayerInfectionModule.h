#pragma once
#include "..\buffer.h"

namespace GamePH {
	class PlayerDI_PH;

	class PlayerInfectionModule {
	public:
		union {
			buffer<0x8, PlayerDI_PH*> pPlayerDI_PH;
			buffer<0x20, float> maxImmunity;
			buffer<0x2C, float> immunity;
		};
		
		static std::vector<PlayerInfectionModule*> playerInfectionModulePtrList;

		static PlayerInfectionModule* Get();
		static void Set(LPVOID instance);

		static void UpdateClassAddr();
	};
}