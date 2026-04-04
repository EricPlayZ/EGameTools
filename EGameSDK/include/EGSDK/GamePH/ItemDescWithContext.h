#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class EGameSDK_API ItemDescWithContext {
	public:
		union {
			DynamicField(ItemDescWithContext, float, weaponDurability);
		};
	};
}