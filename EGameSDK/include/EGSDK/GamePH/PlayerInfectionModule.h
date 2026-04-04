#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class PlayerDI_PH;

	class EGameSDK_API PlayerInfectionModule {
	public:
		union {
			DynamicField(PlayerInfectionModule, PlayerDI_PH*, pPlayerDI_PH);
			DynamicField(PlayerInfectionModule, float, maxImmunity);
			DynamicField(PlayerInfectionModule, float, immunity);
			DynamicField(PlayerInfectionModule, float, nightrunnerTimer);
		};

		~PlayerInfectionModule();
		static PlayerInfectionModule* Get();

		static void UpdateClassAddr();
	};
}
