#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\TimeWeather\CSystem.h>
#include <EGT\Engine\Engine_Hooks.h>
#include <EGT\GamePH\World\WorldRuntime_Internal.h>
#include <EGT\Menu\World.h>

namespace EGT::GamePH::World {
	static bool requestedTimeWeatherInterpolation = false;

	void RequestWorldWeatherInterpolation() {
		requestedTimeWeatherInterpolation = true;
	}

	static void UpdateWeatherIndex() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		const bool weatherDisabledFlag = !iLevel || !iLevel->IsLoaded() || !EGSDK::GamePH::TimeWeather::CSystem::Get();
		if (weatherDisabledFlag)
			Menu::World::weather = EGSDK::GamePH::TimeWeather::EWeather::Default;
	}

	static void UpdateWeatherInterpolation() {
		auto* timeWeather = EGSDK::GamePH::TimeWeather::CSystem::Get();
		if (!timeWeather)
			return;

		static float previousBlendTime = 0.0f;
		static float previousBlendTime2 = 0.0f;
		if (requestedTimeWeatherInterpolation) {
			if (EGSDK::Utils::Values::are_samef(previousBlendTime, 0.0f)) {
				Engine::Hooks::HandleTimeWeatherInterpolationOnDemandTextureIsLoadedHook.Enable();
				previousBlendTime = timeWeather->blendTime;
				previousBlendTime2 = timeWeather->blendTime2;

				timeWeather->blendTime = 1.0f;
				timeWeather->blendTime2 = 1.0f;
				if (!EGSDK::Utils::Memory::IsBadReadPtr(timeWeather->nextSubSystem))
					timeWeather->nextSubSystem->blendTime = 1.0f;
			} else if (!timeWeather->IsFullyBlended()) {
				if (!EGSDK::Utils::Values::are_samef(timeWeather->blendTime, 1.0f) || !EGSDK::Utils::Values::are_samef(timeWeather->blendTime2, 1.0f)) {
					previousBlendTime = timeWeather->blendTime;
					previousBlendTime2 = timeWeather->blendTime2;

					timeWeather->blendTime = 1.0f;
					timeWeather->blendTime2 = 1.0f;
					if (!EGSDK::Utils::Memory::IsBadReadPtr(timeWeather->nextSubSystem))
						timeWeather->nextSubSystem->blendTime = 1.0f;
				} else if (!EGSDK::Utils::Memory::IsBadReadPtr(timeWeather->nextSubSystem) && !EGSDK::Utils::Values::are_samef(timeWeather->nextSubSystem->blendTime, 1.0f)) {
					timeWeather->nextSubSystem->blendTime = 1.0f;
				}
			} else {
				timeWeather->blendTime = previousBlendTime;
				timeWeather->blendTime2 = previousBlendTime2;

				Engine::Hooks::HandleTimeWeatherInterpolationOnDemandTextureIsLoadedHook.Disable();
				requestedTimeWeatherInterpolation = false;
				previousBlendTime = 0.0f;
				previousBlendTime2 = 0.0f;
			}
		}
	}

	void UpdateWorldWeatherRuntime() {
		UpdateWeatherIndex();
		UpdateWeatherInterpolation();
	}
}
