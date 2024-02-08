#pragma once
#include "..\game_classes.h"
#include "menu.h"
	
namespace Menu {
	namespace World {
		extern float time;
		extern float gameSpeed;
		extern KeyBindOption freezeTime;
		extern KeyBindOption slowMotion;
		extern float slowMotionSpeed;
		extern float slowMotionTransitionTime;

		extern EWeather::TYPE weather;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("World", 3) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}