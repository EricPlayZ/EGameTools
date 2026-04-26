#include <memory>
#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\GamePH\GamePH_Misc.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGT\GamePH\Player\PlayerRuntime_Internal.h>
#include <EGT\Menu\Player.h>

namespace EGT::GamePH::Player {
	static void ManagePlayerVars() {
		if (!EGSDK::GamePH::PlayerVariables::gotPlayerVars)
			return;

		EGSDK::GamePH::PlayerVariables::ManageVarByBool("NightRunnerItemForced", true, false, Menu::Player::nightrunnerMode.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("NightRunnerFurySmashEnabled", true, false, Menu::Player::nightrunnerMode.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("NightRunnerFuryGroundPoundEnabled", true, false, Menu::Player::nightrunnerMode.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("AntizinDrainBlocked", true, false, Menu::Player::unlimitedImmunity.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("InfiniteStamina", true, false, Menu::Player::unlimitedStamina.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("UvFlashlightEnergyDrainFactor", 0.0f, 1.0f, Menu::Player::unlimitedItems.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("DamageMulAll", 99999.0f, 0.0f, Menu::Player::oneHitKill.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("InVisibleToEnemies", true, false, Menu::Player::invisibleToEnemies.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("LeftHandDisabled", true, false, Menu::Player::oneHandedMode.GetValue());
	}

	static void PlayerVarListValuesUpdate() {
		if (!Menu::Player::playerVariables.GetValue())
			return;

		EGSDK::GamePH::PlayerVariables::customVars.ForEach([](std::unique_ptr<EGSDK::GamePH::PlayerVar>& customPlayerVarPtr) {
			auto customPlayerVar = EGSDK::GamePH::PlayerVariables::GetVarRefFromPtr(customPlayerVarPtr.get());
			if (!customPlayerVar)
				return;
			if (customPlayerVar->IsManagedByBool())
				return;

			switch (customPlayerVar->GetType()) {
			case EGSDK::Engine::VarType::String:
				break;
			case EGSDK::Engine::VarType::Float: {
				auto customVarValue = customPlayerVar->GetValue<float>();
				if (!customVarValue)
					return;
				auto playerVar = EGSDK::GamePH::PlayerVariables::GetVarRef(customPlayerVar->GetName());
				if (playerVar)
					playerVar->SetValueFromList(*customVarValue);
				break;
			}
			case EGSDK::Engine::VarType::Bool: {
				auto customVarValue = customPlayerVar->GetValue<bool>();
				if (!customVarValue)
					return;
				auto playerVar = EGSDK::GamePH::PlayerVariables::GetVarRef(customPlayerVar->GetName());
				if (playerVar)
					playerVar->SetValueFromList(*customVarValue);
				break;
			}
			default:
				break;
			}
		});
	}

	void UpdatePlayerVarsRuntime() {
		ManagePlayerVars();
		PlayerVarListValuesUpdate();
	}

	void UpdatePlayerOptionStateRuntime() {
		Menu::Player::freezePlayer.SetChangesAreDisabled(!EGSDK::Engine::CBulletPhysicsCharacter::Get());

		if (Menu::Player::disableAirControl.HasChanged()) {
			Menu::Player::disableAirControl.SetPrevValue(Menu::Player::disableAirControl.GetValue());
			EGSDK::GamePH::ReloadJumps();
		}
	}
}
