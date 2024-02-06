#pragma once
#include "..\game_classes.h"
#include "menu.h"
	
namespace Menu {
	namespace World {
		extern float time;
		extern KeyBindOption freezeTime;

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