#pragma once
#include "menu.h"
	
namespace Menu {
	namespace Misc {
		extern KeyBindOption disableGamePauseWhileAFK;
		extern KeyBindOption disableHUD;
		extern Option disableSavegameCRCCheck;
		extern Option disableDataPAKsCRCCheck;
		extern Option increaseDataPAKsLimit;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Misc", 2) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}