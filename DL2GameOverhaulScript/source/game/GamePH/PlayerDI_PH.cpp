#include <pch.h>
#include "..\offsets.h"
#include "LevelDI.h"
#include "PlayerDI_PH.h"

namespace GamePH {
	bool PlayerDI_PH::IsInAir() {
		bool(*pIsInAir)(LPVOID instance) = (decltype(pIsInAir))Offsets::Get_IsInAir();
		if (!pIsInAir)
			return false;

		return pIsInAir(this);
	}

	PlayerDI_PH* PlayerDI_PH::Get() {
		__try {
			LevelDI* iLevel = LevelDI::Get();
			if (!iLevel)
				return nullptr;

			PlayerDI_PH* ptr = iLevel->pPlayerDI_PH;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}