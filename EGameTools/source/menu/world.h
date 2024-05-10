#pragma once
#include "..\game\GamePH\TimeWeather\EWeather.h"
#include "menu.h"
	
namespace Menu {
	namespace World {
		extern float time;
		extern float gameSpeed;
		extern KeyBindOption freezeTime;
		extern KeyBindOption slowMotion;
		extern float slowMotionSpeed;
		extern float slowMotionTransitionTime;

		extern GamePH::TimeWeather::EWeather::TYPE weather;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("World", 5) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}