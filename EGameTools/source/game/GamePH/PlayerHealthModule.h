#pragma once
#include "..\buffer.h"

namespace GamePH {
	class PlayerDI_PH;

	class PlayerHealthModule {
	public:
		union {
			buffer<0x8, PlayerDI_PH*> pPlayerDI_PH;
			buffer<0x2C, float> health;
			buffer<0x3C, float> maxHealth;
		};
		
		static std::vector<PlayerHealthModule*> playerHealthModulePtrList;

		static PlayerHealthModule* Get();
		static void Set(LPVOID instance);

		static void UpdateClassAddr();
	};
}