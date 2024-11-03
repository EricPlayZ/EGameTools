#include <pch.h>
#include "..\offsets.h"
#include "InventoryContainerDI.h"
#include "InventoryItem.h"
#include "LocalClientDI.h"
#include "PlayerDI_PH.h"

namespace GamePH {
	PlayerDI_PH* PlayerDI_PH::Get() {
		__try {
			LocalClientDI* localClient = LocalClientDI::Get();
			if (!localClient)
				return nullptr;

			PlayerDI_PH* ptr = localClient->pPlayerDI_PH;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_PlayerDI_PH())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}

	InventoryItem* PlayerDI_PH::GetCurrentWeapon(UINT indexMaybe) {
		__try {
			LPVOID(*pPlayerGetCurrentWeapon)(LPVOID pPlayerDI_PH, UINT indexMaybe) = (decltype(pPlayerGetCurrentWeapon))Offsets::Get_PlayerGetCurrentWeapon();
			if (!pPlayerGetCurrentWeapon)
				return nullptr;

			InventoryItem* ptr = reinterpret_cast<InventoryItem*>(pPlayerGetCurrentWeapon(this, indexMaybe));
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_InventoryItem())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	InventoryContainerDI* PlayerDI_PH::GetInventoryContainer() {
		__try {
			InventoryContainerDI* ptr = reinterpret_cast<InventoryContainerDI*>(*reinterpret_cast<DWORD64*>(reinterpret_cast<DWORD64>(this) + 0x470));
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_InventoryContainerDI())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}