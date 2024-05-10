#pragma once
#include "..\buffer.h"

namespace GamePH {
	class ItemDescWithContext {
	public:
		union {
			buffer<0xA8, float> weaponDurability;
		};
	};
}