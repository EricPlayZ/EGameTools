#pragma once
#include "..\core.h"
#include "menu.h"

namespace Menu {
	namespace Player {
		extern float playerHealth;
		extern float playerMaxHealth;
		extern float playerImmunity;
		extern float playerMaxImmunity;
		extern int oldWorldMoney;
		extern KeyBindOption godMode;
		extern KeyBindOption freezePlayer;
		extern KeyBindOption unlimitedImmunity;
		extern KeyBindOption unlimitedStamina;
		extern KeyBindOption unlimitedItems;
		extern KeyBindOption oneHitKill;
		extern KeyBindOption invisibleToEnemies;
		extern KeyBindOption disableOutOfBoundsTimer;
		extern KeyBindOption nightrunnerMode;
		extern KeyBindOption oneHandedMode;
		extern KeyBindOption allowGrappleHookInSafezone;
		extern KeyBindOption disableAirControl;
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