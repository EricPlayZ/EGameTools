#include <Hotkey.h>
#include <ImGuiEx.h>
#include <imgui.h>
#include "..\game_classes.h"
#include "world.h"

namespace Menu {
	namespace World {
		float time = 0.0f;
		static float timeBeforeFreeze = 0.0f;
		KeyBindOption freezeTime{ VK_NONE };

		EWeather::TYPE weather = EWeather::TYPE::Default;
		static const char* const weatherItems[7] = {
			"Default",
			"Foggy",
			"Clear",
			"Overcast",
			"Cloudy",
			"Rainy",
			"Stormy"
		};

		Tab Tab::instance{};
		void Tab::Update() {
			GamePH::DayNightCycle* dayNightCycle = GamePH::DayNightCycle::Get();
			if (!dayNightCycle)
				return;
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return;

			if (freezeTime.HasChangedTo(true)) {
				timeBeforeFreeze = time;
				freezeTime.SetPrevValue(true);
			}

			if (!menuToggle.GetValue()) {
				if (!freezeTime.GetValue()) {
					if (freezeTime.HasChangedTo(false)) {
						dayNightCycle->SetDaytime(time);
						freezeTime.SetPrevValue(false);
					}
					time = dayNightCycle->time1 * 24;
				} else if (!Utils::are_same(time, timeBeforeFreeze))
					dayNightCycle->SetDaytime(timeBeforeFreeze);
			}
		}
		void Tab::Render() {
			GamePH::DayNightCycle* dayNightCycle = GamePH::DayNightCycle::Get();
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			ImGui::SeparatorText("Misc##World");
			ImGui::BeginDisabled(!iLevel || !iLevel->IsLoaded() || !dayNightCycle || dayNightCycle->time1 == 0.0f); {
				static bool sliderBeingPressed = false;
				if (ImGui::SliderFloat("Time", &time, 0.01f, 24.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp) && dayNightCycle)
					sliderBeingPressed = true;
				else if (dayNightCycle) {
					if (sliderBeingPressed) {
						sliderBeingPressed = false;
						timeBeforeFreeze = time;
						dayNightCycle->SetDaytime(time);
					}
					if (!freezeTime.GetValue()) {
						if (freezeTime.HasChangedTo(false)) {
							dayNightCycle->SetDaytime(timeBeforeFreeze);
							freezeTime.SetPrevValue(false);
						}
						time = dayNightCycle->time1 * 24;
					} else if (!Utils::are_same(time, timeBeforeFreeze))
						dayNightCycle->SetDaytime(timeBeforeFreeze);
				}

				ImGui::Checkbox("Freeze Time", &freezeTime);
				ImGui::Hotkey("##FreezeTimeToggleKey", freezeTime);
				ImGui::EndDisabled();
			}

			GamePH::TimeWeather::CSystem* timeWeatherSystem = GamePH::TimeWeather::CSystem::Get();
			const bool weatherDisabledFlag = !iLevel || !iLevel->IsLoaded() || !timeWeatherSystem;

			ImGui::SeparatorText("Weather");
			ImGui::BeginDisabled(weatherDisabledFlag); {
				if (ImGui::Combo("Weather", reinterpret_cast<int*>(&weather), weatherItems, IM_ARRAYSIZE(weatherItems)) && timeWeatherSystem)
					timeWeatherSystem->SetForcedWeather(static_cast<EWeather::TYPE>(weather - 1));
				ImGui::Text("Setting weather to: %s", !weatherDisabledFlag ? weatherItems[weather] : "");
				ImGui::Text("Current weather: %s", !weatherDisabledFlag ? weatherItems[timeWeatherSystem->GetCurrentWeather() + 1] : "");
				ImGui::EndDisabled();
			}
		}
	}
}