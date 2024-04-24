#include <pch.h>
#include "..\Engine\CGame.h"
#include "GameDI_PH.h"
#include "..\offsets.h"

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
	void GameDI_PH::TogglePhotoMode(bool doNothing, bool setAsOptionalCamera) {
		__try {
			void(*pTogglePhotoMode)(LPVOID pGameDI_PH, bool doNothing, bool setAsOptionalCamera) = (decltype(pTogglePhotoMode))Offsets::Get_TogglePhotoMode2();
			if (!pTogglePhotoMode)
				return;

			pTogglePhotoMode(this, doNothing, setAsOptionalCamera);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return;
		}
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