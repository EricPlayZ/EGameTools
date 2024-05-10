#include <pch.h>
#include "..\offsets.h"
#include "InventoryContainerDI.h"
#include "InventoryMoney.h"

namespace GamePH {
	InventoryMoney* InventoryContainerDI::GetInventoryMoney(UINT indexMaybe) {
		__try {
			LPVOID(*pPlayerGetInventoryMoney)(LPVOID pPlayerDI_PH, UINT indexMaybe) = (decltype(pPlayerGetInventoryMoney))Offsets::Get_PlayerGetInventoryMoney();
			if (!pPlayerGetInventoryMoney)
				return nullptr;

			InventoryMoney* ptr = reinterpret_cast<InventoryMoney*>(pPlayerGetInventoryMoney(this, indexMaybe));
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_InventoryMoney())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}