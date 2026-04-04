#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class EGameSDK_API InventoryMoney {
	public:
		union {
			DynamicField(InventoryMoney, int, oldWorldMoney);
		};
	};
}