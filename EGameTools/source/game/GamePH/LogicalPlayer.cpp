#include <pch.h>
#include "..\Engine\CGSObject2.h"
#include "LogicalPlayer.h"

namespace GamePH {
	LogicalPlayer* LogicalPlayer::Get() {
		__try {
			Engine::CGSObject2* pCGSObject2 = Engine::CGSObject2::Get();
			if (!pCGSObject2)
				return nullptr;

			LogicalPlayer* ptr = pCGSObject2->pLogicalPlayer;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}