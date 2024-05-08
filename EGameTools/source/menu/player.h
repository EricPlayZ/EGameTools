#pragma once
#include "..\core.h"
#include "menu.h"

namespace Menu {
	namespace Player {
		extern float playerHealth;
		extern float playerMaxHealth;
		extern float playerImmunity;
		extern float playerMaxImmunity;
		extern KeyBindOption godMode;
		extern KeyBindOption unlimitedImmunity;
		extern KeyBindOption unlimitedStamina;
		extern KeyBindOption freezePlayer;
		extern KeyBindOption invisibleToEnemies;
		extern KeyBindOption disableOutOfBoundsTimer;
		extern KeyBindOption nightrunnerMode;
		extern KeyBindOption oneHandedMode;
		extern KeyBindOption disableAirControl;
		extern KeyBindOption allowGrappleHookInSafezone;
		extern Option playerVariables;

		extern std::string saveSCRPath;
		extern std::string loadSCRFilePath;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Player", 0) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}