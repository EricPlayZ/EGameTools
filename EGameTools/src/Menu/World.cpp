#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex.h>
#include <EGSDK\GamePH\DayNightCycle.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGSDK\GamePH\TimeWeather\CSystem.h>
#include <EGSDK\GamePH\TimeWeather\EWeather.h>
#include <EGT\GamePH\World\PrefabSpawnRuntime.h>
#include <EGT\GamePH\World\WorldRuntime.h>
#include <EGT\Menu\World.h>

namespace EGT::Menu {
	namespace World {
		float time = 0.0f;
		float gameSpeed = 1.0f;
		ImGui::KeyBindOption freezeTime{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "FreezeTimeToggleKey" } };
		ImGui::KeyBindOption slowMotion{ false, '4', ImGui::ConfigBindingInfo{ "Menu:Keybinds", "SlowMotionToggleKey" } };
		ImGui::KeyBindOption freezeGameSpeed{ false, '5', ImGui::ConfigBindingInfo{ "Menu:Keybinds", "FreezeGameSpeedToggleKey" }, ImGui::ConfigBindingInfo{ "World:Time", "FreezeGameSpeed" } };
		Config::ConfigFloat slowMotionSpeed{ "World:Time", "SlowMotionSpeed", 0.4f };
		Config::ConfigFloat slowMotionTransitionTime{ "World:Time", "SlowMotionTransitionTime", 1.0f };

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

		ImGui::KeyBindOption enableSnow{ false, VK_NONE };
		ImGui::KeyBindOption dynamicSnow{ false, VK_NONE };
		float snowCover = 0.5f;
		float snowStrength = 0.0f;
		float snowCoverAdditionalMax = 0.2f;
		float snowCoverTime = 120.0f;

		Tab Tab::instance{};
		void Tab::Init() {}
		void Tab::Update() {
			GamePH::World::UpdateRuntimeState();
		}

		void Tab::Render() {
			auto* dayNightCycle = EGSDK::GamePH::DayNightCycle::Get();
			auto* timeWeatherSystem = EGSDK::GamePH::TimeWeather::CSystem::Get();
			auto* iLevel = EGSDK::GamePH::LevelDI::Get();

			ImGui::SeparatorTextSection("Time##World", false);
			ImGui::BeginDisabled(!iLevel || !iLevel->IsLoaded() || !dayNightCycle || !timeWeatherSystem);
			const bool timeSlider = ImGui::SliderFloatStacked("Time", &time, 0.01f, 24.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("AntizinDrainBlocked", true, false, timeSlider);
			if (timeSlider) {
				GamePH::World::OnTimeSliderChanged(time);
				dayNightCycle->SetDaytime(time);
			}

			ImGui::BeginDisabled(slowMotion.GetValue() || freezeGameSpeed.GetValue());
			const bool isModifyingGameSpeed = ImGui::SliderFloatStacked("Game Speed", &gameSpeed, 0.0f, 2.0f, "%.2fx");
			GamePH::World::SetIsModifyingGameSpeed(isModifyingGameSpeed);
			if (isModifyingGameSpeed)
				iLevel->TimerSetSpeedUp(gameSpeed);
			ImGui::EndDisabled();

			ImGui::CheckboxHotkey("Freeze Time", &freezeTime, "Freezes time");
			ImGui::BeginDisabled(freezeGameSpeed.GetValue());
			ImGui::CheckboxHotkey("Slow Motion", &slowMotion, "Slows the game down to the speed specified on the \"Slow Motion Speed\" slider");
			ImGui::EndDisabled();
			ImGui::BeginDisabled(slowMotion.GetValue());
			ImGui::CheckboxHotkey("Freeze Game Speed", &freezeGameSpeed, "Near-freezes gameplay using a near-zero game speed (does not use level timer freeze)");
			ImGui::EndDisabled();
			ImGui::EndDisabled();

			ImGui::SliderFloatStacked("Slow Motion Speed", &slowMotionSpeed, 0.01f, 0.99f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloatStacked("Slow Motion Transition Time", &slowMotionTransitionTime, 0.00f, 5.00f, "%.2fs", ImGuiSliderFlags_AlwaysClamp);

			const bool weatherDisabledFlag = !iLevel || !iLevel->IsLoaded() || !timeWeatherSystem;
			ImGui::SeparatorTextSection("Weather##World");
			ImGui::BeginDisabled(weatherDisabledFlag);
			if (ImGui::ComboStacked("Weather", reinterpret_cast<int*>(&weather), weatherItems, IM_ARRAYSIZE(weatherItems))) {
				GamePH::World::RequestWeatherInterpolation();
				timeWeatherSystem->SetForcedWeather(static_cast<EGSDK::GamePH::TimeWeather::EWeather>(weather - 1));
			}
			ImGui::Text("Current weather: %s", !weatherDisabledFlag ? weatherItems[timeWeatherSystem->GetCurrentWeather() + 1] : "");
			ImGui::EndDisabled();

			ImGui::CheckboxHotkey("Enable Snow", &enableSnow, "Enable snow surface on the map");
			ImGui::CheckboxHotkey("Dynamic Snow", &dynamicSnow, "Should snow follow weather and gather on surface dynamically over time");
			ImGui::SliderFloatStacked("Snow Cover", &snowCover, 0.0f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp, "Base amount of snow cover. On dynamic snow settings snow cover can get additional snow, but never gets smaller than this setting");
			ImGui::SliderFloatStacked("Snow Cover Additional Max", &snowCoverAdditionalMax, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp, "Maximum amount of additional snow cover. On dynamic settings additional snow cover gathers as snow continues to fall");
			ImGui::SliderFloatStacked("Snow Cover Time", &snowCoverTime, 0.0f, 2000.0f, "%.2fs", ImGuiSliderFlags_AlwaysClamp, "How long should it take for snow cover to reach its maximum state");
			ImGui::SliderFloatStacked("Snow Strength", &snowStrength, 0.0f, 1, "%.2fs", ImGuiSliderFlags_AlwaysClamp);

			EGT::GamePH::World::PrefabSpawnRuntime::Render();
		}
	}
}
