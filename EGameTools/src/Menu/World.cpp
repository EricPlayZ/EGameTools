#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex.h>
#include <ImGui\imguiex_animation.h>
#include <EGSDK\GamePH\DayNightCycle.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGSDK\GamePH\TimeWeather\CSystem.h>
#include <EGSDK\GamePH\TimeWeather\EWeather.h>
#include <EGT\ImGui_impl\NextFrameTask.h>
#include <EGT\Engine\Engine_Hooks.h>
#include <EGT\Menu\World.h>

namespace EGT::Menu {
	namespace World {
		float time = 0.0f;
		static float timeBeforeFreeze = 0.0f;
		float gameSpeed = 1.0f;
		static bool isModifyingGameSpeed = false;
		static float actualGameSpeed = gameSpeed;
		static float gameSpeedBeforeSlowMo = gameSpeed;
		ImGui::KeyBindOption freezeTime{ false, VK_NONE };
		ImGui::KeyBindOption slowMotion{ false, '4' };
		float slowMotionSpeed = 0.4f;
		static float slowMotionSpeedLerp = gameSpeed;
		float slowMotionTransitionTime = 1.0f;

		EGSDK::GamePH::TimeWeather::EWeather weather = EGSDK::GamePH::TimeWeather::EWeather::Default;
		static const char* const weatherItems[7] = {
			"Default",
			"Foggy",
			"Clear",
			"Overcast",
			"Cloudy",
			"Rainy",
			"Stormy"
		};
		static bool requestedTimeWeatherInterpolation = false;

		static void UpdateFreezeTime() {
			auto dayNightCycle = EGSDK::GamePH::DayNightCycle::Get();
			if (!dayNightCycle)
				return;
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return;

			if (freezeTime.HasChangedTo(true)) {
				timeBeforeFreeze = time;
				freezeTime.SetPrevValue(true);
			} else if (freezeTime.HasChangedTo(false)) {
				dayNightCycle->SetDaytime(timeBeforeFreeze);
				freezeTime.SetPrevValue(false);
			}

			time = dayNightCycle->time1 * 24.0f;
			if (freezeTime.GetValue() && !EGSDK::Utils::Values::are_samef(time, timeBeforeFreeze, 0.0095f))
				dayNightCycle->SetDaytime(timeBeforeFreeze);
		}
		static void UpdateSlowMo() {
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return;

			static bool slowMoHasChanged = true;
			if (slowMotion.HasChangedTo(false)) {
				static float gameSpeedAfterChange = 0.0f;
				if (slowMoHasChanged)
					gameSpeedAfterChange = actualGameSpeed;

				slowMotionSpeedLerp = ImGui::AnimateLerp("slowMotionSpeedLerp", gameSpeedAfterChange, gameSpeedBeforeSlowMo, slowMotionTransitionTime, slowMoHasChanged, &ImGui::AnimEaseInOutSine);
				iLevel->TimerSetSpeedUp(slowMotionSpeedLerp);
				slowMoHasChanged = false;

				if (EGSDK::Utils::Values::are_samef(actualGameSpeed, gameSpeedBeforeSlowMo)) {
					slowMoHasChanged = true;
					slowMotion.SetPrevValue(false);
				}
			} else if (slowMotion.GetValue()) {
				static float gameSpeedAfterChange = 0.0f;
				if (slowMotion.HasChanged()) {
					if (slowMoHasChanged)
						gameSpeedBeforeSlowMo = actualGameSpeed;
					gameSpeedAfterChange = actualGameSpeed;
				}

				slowMotionSpeedLerp = ImGui::AnimateLerp("slowMotionSpeedLerp", gameSpeedAfterChange, slowMotionSpeed, slowMotionTransitionTime, slowMotion.HasChanged(), &ImGui::AnimEaseInOutSine);
				iLevel->TimerSetSpeedUp(slowMotionSpeedLerp);

				if (slowMotion.HasChanged()) {
					slowMoHasChanged = true;
					slowMotion.SetPrevValue(slowMotion.GetValue());
				}
			}

			if (!isModifyingGameSpeed) {
				if (!slowMotion.GetValue() && !slowMotion.HasChanged() && !EGSDK::Utils::Values::are_samef(gameSpeed, 1.0f))
					iLevel->TimerSetSpeedUp(gameSpeed);
				actualGameSpeed = iLevel->TimerGetSpeedUp();
			}
		}
		static void UpdateWeatherIndex() {
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			bool weatherDisabledFlag = !iLevel || !iLevel->IsLoaded() || !EGSDK::GamePH::TimeWeather::CSystem::Get();
			if (weatherDisabledFlag)
				weather = EGSDK::GamePH::TimeWeather::EWeather::Default;
		}
		static void UpdateWeatherInterpolation() {
			auto timeWeather = EGSDK::GamePH::TimeWeather::CSystem::Get();
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
					} else if (!EGSDK::Utils::Memory::IsBadReadPtr(timeWeather->nextSubSystem) && !EGSDK::Utils::Values::are_samef(timeWeather->nextSubSystem->blendTime, 1.0f))
						timeWeather->nextSubSystem->blendTime = 1.0f;
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

		Tab Tab::instance{};
		void Tab::Init() {}
		void Tab::Update() {
			UpdateFreezeTime();
			UpdateSlowMo();
			UpdateWeatherIndex();
			UpdateWeatherInterpolation();
		}
		void Tab::Render() {
			auto dayNightCycle = EGSDK::GamePH::DayNightCycle::Get();
			auto timeWeatherSystem = EGSDK::GamePH::TimeWeather::CSystem::Get();
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			ImGui::SeparatorText("Time##World");
			ImGui::BeginDisabled(!iLevel || !iLevel->IsLoaded() || !dayNightCycle || !timeWeatherSystem);
			bool timeSlider = ImGui::SliderFloat("Time", &time, 0.01f, 24.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("AntizinDrainBlocked", true, false, timeSlider);
			if (timeSlider) {
				requestedTimeWeatherInterpolation = true;
				timeBeforeFreeze = time;
				dayNightCycle->SetDaytime(time);
			}

			ImGui::BeginDisabled(slowMotion.GetValue()); {
				isModifyingGameSpeed = ImGui::SliderFloat("Game Speed", &gameSpeed, 0.0f, 2.0f, "%.2fx");
				if (isModifyingGameSpeed)
					iLevel->TimerSetSpeedUp(gameSpeed);
				ImGui::EndDisabled();
			}

			ImGui::CheckboxHotkey("Freeze Time", &freezeTime, "Freezes time");
			ImGui::SameLine();
			ImGui::CheckboxHotkey("Slow Motion", &slowMotion, "Slows the game down to the speed specified on the \"Slow Motion Speed\" slider");
			ImGui::EndDisabled();

			ImGui::SliderFloat("Slow Motion Speed", &slowMotionSpeed, 0.01f, 0.99f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Slow Motion Transition Time", &slowMotionTransitionTime, 0.00f, 5.00f, "%.2fs", ImGuiSliderFlags_AlwaysClamp);

			bool weatherDisabledFlag = !iLevel || !iLevel->IsLoaded() || !timeWeatherSystem;
			ImGui::SeparatorText("Weather##World");
			ImGui::BeginDisabled(weatherDisabledFlag);
			if (ImGui::Combo("Weather", reinterpret_cast<int*>(&weather), weatherItems, IM_ARRAYSIZE(weatherItems))) {
				requestedTimeWeatherInterpolation = true;
				timeWeatherSystem->SetForcedWeather(static_cast<EGSDK::GamePH::TimeWeather::EWeather>(weather - 1));
			}
			ImGui::Text("Current weather: %s", !weatherDisabledFlag ? weatherItems[timeWeatherSystem->GetCurrentWeather() + 1] : "");
			ImGui::EndDisabled();
		}
	}
}