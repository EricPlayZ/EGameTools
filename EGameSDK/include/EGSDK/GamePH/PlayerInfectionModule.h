#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class PlayerDI_PH;

	class EGameSDK_API PlayerInfectionModule {
	public:
		union {
			ClassHelpers::StaticBuffer<0x8, PlayerDI_PH*> pPlayerDI_PH;
			ClassHelpers::StaticBuffer<0x20, float> maxImmunity;
			ClassHelpers::StaticBuffer<0x2C, float> immunity;
			ClassHelpers::StaticBuffer<0x98, float> nightrunnerTimer;
		};

		~PlayerInfectionModule();
		static PlayerInfectionModule* Get();

		static void UpdateClassAddr();
	};
}
