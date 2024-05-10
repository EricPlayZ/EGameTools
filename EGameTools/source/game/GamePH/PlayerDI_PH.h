#pragma once
#include "..\buffer.h"
#include "InventoryItem.h"

namespace GamePH {
	class PlayerDI_PH {
	public:
		static PlayerDI_PH* Get();

		InventoryItem* GetCurrentWeapon(UINT a1);
	};
}