#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class PlayerDI_PH;

	class EGameSDK_API PlayerHealthModule {
	public:
		union {
			DynamicField(PlayerHealthModule, PlayerDI_PH*, pPlayerDI_PH);
			DynamicField(PlayerHealthModule, float, health);
			DynamicField(PlayerHealthModule, float, maxHealth);
		};

		~PlayerHealthModule();
		static PlayerHealthModule* Get();

		static void UpdateClassAddr();
	};
}
