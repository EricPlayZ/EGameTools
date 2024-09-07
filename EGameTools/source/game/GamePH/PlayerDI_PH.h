#pragma once
#include "..\buffer.h"
#include "InventoryItem.h"
#include "InventoryContainerDI.h"

namespace GamePH {
	class PlayerDI_PH {
	public:
		union {
			buffer<0x35E1, bool> enableTPPModel1;
			buffer<0x35E2, bool> enableTPPModel2;
		};

		static PlayerDI_PH* Get();

		InventoryItem* GetCurrentWeapon(UINT indexMaybe);
		InventoryContainerDI* GetInventoryContainer();
	};
}