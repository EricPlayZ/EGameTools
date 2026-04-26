#include <cfloat>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerHealthModule.h>
#include <EGSDK\GamePH\PlayerInfectionModule.h>
#include <EGT\GamePH\Player\PlayerRuntime_Internal.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\Player.h>

namespace EGT::GamePH::Player {
	void UpdatePlayerStatsRuntime() {
		auto* playerHealthModule = EGSDK::GamePH::PlayerHealthModule::Get();
		if (playerHealthModule) {
			Menu::Player::playerMaxHealth = playerHealthModule->maxHealth;
			if (!Menu::menuToggle.GetValue()) {
				auto* iLevel = EGSDK::GamePH::LevelDI::Get();
				if (iLevel && iLevel->IsLoaded())
					Menu::Player::playerHealth = playerHealthModule->health;
			}
		}

		auto* playerInfectionModule = EGSDK::GamePH::PlayerInfectionModule::Get();
		if (!playerInfectionModule)
			return;

		static float previousNightrunnerTimer = playerInfectionModule->nightrunnerTimer;
		if (Menu::Player::nightrunnerMode.GetValue()) {
			if (Menu::Player::nightrunnerMode.HasChanged()) {
				previousNightrunnerTimer = playerInfectionModule->nightrunnerTimer;
				Menu::Player::nightrunnerMode.SetPrevValue(true);
			}
			playerInfectionModule->nightrunnerTimer = FLT_MAX;
		} else if (Menu::Player::nightrunnerMode.HasChanged()) {
			playerInfectionModule->nightrunnerTimer = previousNightrunnerTimer;
			Menu::Player::nightrunnerMode.SetPrevValue(false);
		}

		Menu::Player::playerMaxImmunity = playerInfectionModule->maxImmunity * 100.0f;
		if (Menu::menuToggle.GetValue())
			return;

		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;
		Menu::Player::playerImmunity = playerInfectionModule->immunity * 100.0f;
	}
}
