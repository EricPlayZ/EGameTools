#include <pch.h>
#include "CGame.h"
#include "CLobbySteam.h"

namespace Engine {
	CGame* CGame::Get() {
		__try {
			CLobbySteam* pCLobbySteam = CLobbySteam::Get();
			if (!pCLobbySteam)
				return nullptr;

			CGame* ptr = pCLobbySteam->pCGame;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}