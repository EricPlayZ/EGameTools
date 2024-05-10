#pragma once
#include "menu.h"
	
namespace Menu {
	namespace Debug {
		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Debug", 5) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}