#pragma once
#include "..\buffer.h"

namespace GamePH {
	class InventoryMoney {
	public:
		buffer<0x38, int> oldWorldMoney;
	};
}