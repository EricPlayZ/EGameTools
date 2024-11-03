#include <pch.h>
#include "..\Engine\CLevel.h"
#include "..\offsets.h"
#include "LevelDI.h"
#include "PlayerDI_PH.h"

namespace GamePH {
	bool LevelDI::IsLoading() {
		__try {
			bool(*pIsLoading)(LPVOID iLevel) = (decltype(pIsLoading))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?IsLoading@ILevel@@QEBA_NXZ");
			if (!pIsLoading)
				return true;

			return pIsLoading(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return true;
		}
	}
	bool LevelDI::IsLoaded() {
		static Utils::Time::Timer loadTimer{ 7500 };
		static bool isStillLoading = false;

		if (IsLoading() || !GamePH::PlayerDI_PH::Get()) {
			isStillLoading = true;
			return false;
		}
		if (isStillLoading) {
			isStillLoading = false;
			loadTimer = Utils::Time::Timer(7500);
		}

		return loadTimer.DidTimePass();
	}
	LPVOID LevelDI::GetViewCamera() {
		__try {
			LPVOID(*pGetViewCamera)(LPVOID iLevel) = (decltype(pGetViewCamera))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?GetViewCamera@ILevel@@QEBAPEAVIBaseCamera@@XZ");
			if (!pGetViewCamera)
				return nullptr;

			return pGetViewCamera(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	float LevelDI::GetTimeDelta() {
		__try {
			float(*pGetTimeDelta)(LPVOID iLevel) = (decltype(pGetTimeDelta))Utils::Memory::GetProcAddr("gamedll_ph_x64_rwdi.dll", "?GetTimeDelta@IGSObject@@UEBAMXZ");
			if (!pGetTimeDelta)
				return 0.0f;

			return pGetTimeDelta(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return 0.0f;
		}
	}
	void LevelDI::SetViewCamera(LPVOID viewCam) {
		__try {
			void(*pSetViewCamera)(LPVOID iLevel, LPVOID viewCam) = (decltype(pSetViewCamera))Utils::Memory::GetProcAddr("gamedll_ph_x64_rwdi.dll", "?SetViewCamera@ILevel@@UEAAXPEAVIBaseCamera@@@Z_0");
			if (!pSetViewCamera)
				return;

			pSetViewCamera(this, viewCam);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return;
		}
	}
	float LevelDI::GetTimePlayed() {
		__try {
			float(*pGetTimePlayed)(LPVOID iLevel) = (decltype(pGetTimePlayed))Utils::Memory::GetProcAddr("gamedll_ph_x64_rwdi.dll", "?GetTimePlayed@ILevel@@UEBAMXZ");
			if (!pGetTimePlayed)
				return 0.0f;

			return pGetTimePlayed(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return 0.0f;
		}
	}
	void LevelDI::ShowUIManager(bool enabled) {
		__try {
			void(*pShowUIManager)(LPVOID iLevel, bool enabled) = (decltype(pShowUIManager))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?ShowUIManager@ILevel@@QEAAX_N@Z");
			if (!pShowUIManager)
				return;

			pShowUIManager(this, enabled);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return;
		}
	}
	bool LevelDI::IsTimerFrozen() {
		__try {
			bool(*pIsTimerFrozen)(LPVOID iLevel) = (decltype(pIsTimerFrozen))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?IsTimerFrozen@ILevel@@QEBA_NXZ");
			if (!pIsTimerFrozen)
				return false;

			return pIsTimerFrozen(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}
	float LevelDI::TimerGetSpeedUp() {
		__try {
			float(*pTimerGetSpeedUp)(LPVOID iLevel) = (decltype(pTimerGetSpeedUp))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?TimerGetSpeedUp@ILevel@@QEBAMXZ");
			if (!pTimerGetSpeedUp)
				return -1.0f;

			return pTimerGetSpeedUp(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return -1.0f;
		}
	}
	void LevelDI::TimerSetSpeedUp(float timeScale) {
		__try {
			void(*pTimerSetSpeedUp)(LPVOID iLevel, float timeScale) = (decltype(pTimerSetSpeedUp))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?TimerSetSpeedUp@ILevel@@QEAAXM@Z");
			if (!pTimerSetSpeedUp)
				return;

			pTimerSetSpeedUp(this, timeScale);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return;
		}
	}
	TimeWeather::CSystem* LevelDI::GetTimeWeatherSystem() {
		__try {
			TimeWeather::CSystem* (*pGetTimeWeatherSystem)(LevelDI* iLevel) = (decltype(pGetTimeWeatherSystem))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?GetTimeWeatherSystem@ILevel@@QEBAPEAVCSystem@TimeWeather@@XZ");
			if (!pGetTimeWeatherSystem)
				return nullptr;

			return pGetTimeWeatherSystem(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}

	LevelDI* LevelDI::Get() {
		__try {
			Engine::CLevel* pCLevel = Engine::CLevel::Get();
			if (!pCLevel)
				return nullptr;

			LevelDI* ptr = pCLevel->pLevelDI;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_LevelDI())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}