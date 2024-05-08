#include <pch.h>
#include "..\offsets.h"
#include "CGame.h"
#include "CLevel.h"

namespace Engine {
	CLevel* CLevel::Get() {
		__try {
			CGame* pCGame = CGame::Get();
			if (!pCGame)
				return nullptr;

			CLevel* ptr = pCGame->pCLevel;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_CLevel())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}