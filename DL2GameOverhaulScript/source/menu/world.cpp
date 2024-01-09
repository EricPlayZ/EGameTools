#include <imgui.h>
#include "..\game_classes.h"

namespace Menu {
	namespace World {
		float time = 0.0f;

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

		void Render() {
			GamePH::DayNightCycle* dayNightCycle = GamePH::DayNightCycle::Get();
			ImGui::SeparatorText("Misc##World");
			ImGui::BeginDisabled(!Engine::CBulletPhysicsCharacter::Get() || !dayNightCycle || dayNightCycle->time1 == 0.0f); {
				if (ImGui::SliderFloat("Time", &time, 0.01f, 24.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp) && dayNightCycle)
					dayNightCycle->SetDaytime(time);
				else if (dayNightCycle)
					time = dayNightCycle->time1 * 24;
				ImGui::EndDisabled();
			}

			GamePH::TimeWeather::CSystem* timeWeatherSystem = GamePH::TimeWeather::CSystem::Get();
			const bool weatherDisabledFlag = !Engine::CBulletPhysicsCharacter::Get() || !timeWeatherSystem;

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