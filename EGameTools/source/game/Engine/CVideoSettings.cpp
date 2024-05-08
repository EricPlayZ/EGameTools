#include <pch.h>
#include "..\offsets.h"
#include "CGame.h"
#include "CVideoSettings.h"

namespace Engine {
	CVideoSettings* CVideoSettings::Get() {
		__try {
			CGame* pCGame = CGame::Get();
			if (!pCGame)
				return nullptr;

			CVideoSettings* ptr = pCGame->pCVideoSettings;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_CVideoSettings())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}