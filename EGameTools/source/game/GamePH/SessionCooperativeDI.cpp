#include <pch.h>
#include "..\offsets.h"
#include "GameDI_PH.h"
#include "SessionCooperativeDI.h"

namespace GamePH {
	SessionCooperativeDI* SessionCooperativeDI::Get() {
		__try {
			GameDI_PH* pGameDI_PH = GameDI_PH::Get();
			if (!pGameDI_PH)
				return nullptr;

			SessionCooperativeDI* ptr = pGameDI_PH->pSessionCooperativeDI;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_SessionCooperativeDI())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}