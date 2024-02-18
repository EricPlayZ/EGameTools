#include <pch.h>
#include "..\offsets.h"
#include "GameDI_PH.h"
#include "GameDI_PH2.h"

namespace GamePH {
	GameDI_PH2* GameDI_PH2::Get() {
		__try {
			GameDI_PH* pGameDI_PH = GameDI_PH::Get();
			if (!pGameDI_PH)
				return nullptr;

			GameDI_PH2* ptr = reinterpret_cast<GameDI_PH2*>(reinterpret_cast<DWORD64>(pGameDI_PH) + Offsets::Get_gameDI_PH2_offset());
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}