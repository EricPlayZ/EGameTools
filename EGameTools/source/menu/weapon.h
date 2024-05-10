#pragma once
#include "..\core.h"
#include "menu.h"

namespace Menu {
	namespace Weapon {
		extern KeyBindOption unlimitedDurability;
		extern KeyBindOption unlimitedAmmo;
		extern KeyBindOption noSpread;
		extern KeyBindOption noRecoil;
		extern KeyBindOption instantReload;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Weapon", 1) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}