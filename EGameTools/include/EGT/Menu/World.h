#pragma once
#include <EGSDK\GamePH\TimeWeather\EWeather.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Config\ConfigValue.h>
	
namespace EGT::Menu {
	namespace World {
		extern float time;
		extern float gameSpeed;
		extern ImGui::KeyBindOption freezeTime;
		extern ImGui::KeyBindOption slowMotion;
		extern ImGui::KeyBindOption freezeGameSpeed;
		extern Config::ConfigFloat slowMotionSpeed;
		extern Config::ConfigFloat slowMotionTransitionTime;

		extern EGSDK::GamePH::TimeWeather::EWeather weather;
		extern ImGui::KeyBindOption enableSnow;
		extern ImGui::KeyBindOption dynamicSnow;
		extern float snowCover;
		extern float snowStrength;
		extern float snowCoverAdditionalMax;
		extern float snowCoverTime;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("World", 5) {}
			void Init() override;
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}