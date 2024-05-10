#pragma once
#include "..\core.h"
#include "menu.h"

namespace Menu {
	namespace Weapon {
		extern KeyBindOption unlimitedDurability;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Weapon", 1) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}