#include <pch.h>
#include "..\LevelDI.h"
#include "CSystem.h"
#include "EWeather.h"

namespace GamePH {
	namespace TimeWeather {
		void CSystem::SetForcedWeather(int weather) {
			__try {
				void(*pSetForcedWeather)(LPVOID timeWeatherSystem, int weather) = (decltype(pSetForcedWeather))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?SetForcedWeather@CSystem@TimeWeather@@QEAAXW4TYPE@EWeather@@VApiDebugAccess@2@@Z");
				if (!pSetForcedWeather)
					return;

				pSetForcedWeather(this, weather);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return;
			}
		}
		int CSystem::GetCurrentWeather() {
			__try {
				int(*pGetCurrentWeather)(LPVOID timeWeatherSystem) = (decltype(pGetCurrentWeather))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?GetCurrentWeather@CSystem@TimeWeather@@QEBA?AW4TYPE@EWeather@@XZ");
				if (!pGetCurrentWeather)
					return EWeather::TYPE::Default;

				return pGetCurrentWeather(this);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return EWeather::TYPE::Default;
			}
		}

		CSystem* CSystem::Get() {
			__try {
				LevelDI* pLevelDI = LevelDI::Get();
				if (!pLevelDI)
					return nullptr;

				CSystem* ptr = pLevelDI->GetTimeWeatherSystem();
				if (!Utils::Memory::IsValidPtr(ptr))
					return nullptr;

				return ptr;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return nullptr;
			}
		}
	}
}