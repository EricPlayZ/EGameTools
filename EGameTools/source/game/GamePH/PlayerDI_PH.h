#pragma once
#include "..\buffer.h"
#include "InventoryItem.h"
#include "InventoryContainerDI.h"

namespace Engine {
	class CoPhysicsProperty;
}

namespace GamePH {
	class PlayerDI_PH {
	public:
		union {
			buffer<0xF0, Engine::CoPhysicsProperty*> pCoPhysicsProperty;
			buffer<0x35E9, bool> enableTPPModel1;
			buffer<0x35EA, bool> enableTPPModel2;
		};

		static PlayerDI_PH* Get();

		InventoryItem* GetCurrentWeapon(UINT indexMaybe);
		InventoryContainerDI* GetInventoryContainer();
	};
}