#pragma once
#include "..\buffer.h"
#include "InventoryMoney.h"

namespace GamePH {
	class InventoryContainerDI {
	public:
		InventoryMoney* GetInventoryMoney(UINT indexMaybe);
	};
}