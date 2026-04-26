#pragma once
#include <EGT\Core\Core.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Config\ConfigValue.h>

namespace EGT::Menu {
	namespace Player {
		extern float playerHealth;
		extern float playerMaxHealth;
		extern float playerImmunity;
		extern float playerMaxImmunity;
		extern int oldWorldMoney;
		extern ImGui::KeyBindOption godMode;
		extern ImGui::KeyBindOption freezePlayer;
		extern ImGui::KeyBindOption unlimitedImmunity;
		extern ImGui::KeyBindOption unlimitedStamina;
		extern ImGui::KeyBindOption unlimitedItems;
		extern ImGui::KeyBindOption oneHitKill;
		extern ImGui::KeyBindOption invisibleToEnemies;
		extern ImGui::KeyBindOption disableOutOfBoundsTimer;
		extern ImGui::KeyBindOption nightrunnerMode;
		extern ImGui::KeyBindOption oneHandedMode;
		extern ImGui::KeyBindOption disableSafezoneRestrictions;
		extern ImGui::KeyBindOption disableAirControl;
		extern ImGui::Option playerVariables;

		extern Config::ConfigString saveSCRPath;
		extern Config::ConfigString loadSCRFilePath;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Player", 0) {}
			void Init() override;
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}