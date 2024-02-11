#pragma once
#include "..\buffer.h"

namespace GamePH {
	class PlayerHealthModule {
	public:
		union {
			buffer<0x2C, float> health;
			buffer<0x3C, float> maxHealth;
		};
		
		static PlayerHealthModule* pPlayerHealthModule;
		static PlayerHealthModule* Get();
	};
}