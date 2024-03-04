#include <pch.h>
#include "..\Engine\CGame.h"
#include "GameDI_PH.h"

namespace GamePH {
	float GameDI_PH::GetGameTimeDelta() {
		__try {
			float(*pGetGameTimeDelta)(LPVOID pGameDI_PH) = (decltype(pGetGameTimeDelta))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?GetGameTimeDelta@IGame@@QEBAMXZ");
			if (!pGetGameTimeDelta)
				return -1.0f;

			return pGetGameTimeDelta(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return -1.0f;
		}
	}
	DWORD64 GameDI_PH::GetCurrentGameVersion() {
		return Utils::Memory::CallVT<228, DWORD64>(this);
	}
	void GameDI_PH::TogglePhotoMode(bool doNothing, bool setAsOptionalCamera) {
		Utils::Memory::CallVT<260>(this, doNothing, setAsOptionalCamera);
	}

	GameDI_PH* GameDI_PH::Get() {
		__try {
			Engine::CGame* pCGame = Engine::CGame::Get();
			if (!pCGame)
				return nullptr;

			GameDI_PH* ptr = pCGame->pGameDI_PH;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}