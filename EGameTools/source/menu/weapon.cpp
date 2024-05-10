#include <pch.h>
#include "..\core.h"
#include "..\game\GamePH\LevelDI.h"
#include "..\game\GamePH\PlayerDI_PH.h"
#include "..\game\GamePH\PlayerVariables.h"
#include "menu.h"
#include "weapon.h"

namespace Menu {
	namespace Weapon {
		float currentWeaponDurability = 0.0f;
		KeyBindOption unlimitedDurability{ VK_NONE };
		KeyBindOption unlimitedAmmo{ VK_NONE };
		KeyBindOption noSpread{ VK_NONE };
		KeyBindOption noRecoil{ VK_NONE };
		KeyBindOption instantReload{ VK_NONE };

		static constexpr float baseWeaponDurabilityMul = 1.0f;
		static constexpr float baseWeaponAccuracyMul = 1.0f;
		static constexpr float baseBowAccuracyMul = 0.1f;
		static constexpr float baseWeaponRecoilMul = 1.0f;
		static constexpr float baseWeaponReloadMul = 1.0f;

		static void UpdatePlayerVars() {
			if (!GamePH::PlayerVariables::gotPlayerVars)
				return;

			GamePH::PlayerVariables::ManagePlayerVarOption("MeleeWpnDurabilityMulReduce", 0.0f, baseWeaponDurabilityMul, &unlimitedDurability, true);

			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsPistolInfiniteAmmo", true, false, &unlimitedAmmo);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsRevolverInfiniteAmmo", true, false, &unlimitedAmmo);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsRifleInfiniteAmmo", true, false, &unlimitedAmmo);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsShotgunInfiniteAmmo", true, false, &unlimitedAmmo);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsSMGInfiniteAmmo", true, false, &unlimitedAmmo);
			GamePH::PlayerVariables::ManagePlayerVarOption("InfiniteArrows", true, false, &unlimitedAmmo);

			GamePH::PlayerVariables::ManagePlayerVarOption("BulletAccuracyFactor", 0.0f, baseWeaponAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsMoveAccuracyReduce", 0.0f, baseWeaponAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsPistolAccuracyFactor", 0.0f, baseWeaponAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsRevolverAccuracyFactor", 0.0f, baseWeaponAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsRifleAccuracyFactor", 0.0f, baseWeaponAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsShotgunAccuracyFactor", 0.0f, baseWeaponAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsCrossbowAccuracyFactor", 0.0f, baseBowAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsHarpoonAccuracyFactor", 0.0f, baseBowAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("BowAccuracyFactor", 0.0f, baseBowAccuracyMul, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("BowMaxThrowFactor", 99999.0f, 1.0f, &noSpread, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("BowSlowMoAccuracyMul", 0.0f, 0.25f, &noSpread, true);

			GamePH::PlayerVariables::ManagePlayerVarOption("BulletRecoilFactor", 0.0f, baseWeaponRecoilMul, &noRecoil, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsPistolRecoilFactor", 0.0f, baseWeaponRecoilMul, &noRecoil, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsRevolverRecoilFactor", 0.0f, baseWeaponRecoilMul, &noRecoil, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsRifleRecoilFactor", 0.0f, baseWeaponRecoilMul, &noRecoil, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsShotgunRecoilFactor", 0.0f, baseWeaponRecoilMul, &noRecoil, true);

			GamePH::PlayerVariables::ManagePlayerVarOption("BulletReloadSpeed", 1000.0f, 0.0f, &instantReload, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsPistolReloadTimeMul", 1000.0f, baseWeaponReloadMul, &instantReload, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsRevolverReloadTimeMul", 1000.0f, baseWeaponReloadMul, &instantReload, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsRifleReloadTimeMul", 1000.0f, baseWeaponReloadMul, &instantReload, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("FirearmsShotgunReloadTimeMul", 1000.0f, baseWeaponReloadMul, &instantReload, true);
			GamePH::PlayerVariables::ManagePlayerVarOption("BowPutArrowDuration", 0.0f, 0.137f, &instantReload, true);
		}
		static void UpdateWeaponDurability(bool updateSlider) {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return;
			GamePH::PlayerDI_PH* player = GamePH::PlayerDI_PH::Get();
			if (!player)
				return;
			GamePH::InventoryItem* weaponItem = player->GetCurrentWeapon(0);
			if (!weaponItem)
				return;
			GamePH::ItemDescWithContext* weaponItemCtx = weaponItem->GetItemDescCtx();
			if (!weaponItemCtx)
				return;

			updateSlider ? (currentWeaponDurability = weaponItemCtx->weaponDurability) : (weaponItemCtx->weaponDurability = currentWeaponDurability);
		}

		static bool isWeaponInteractionDisabled() {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return true;
			GamePH::PlayerDI_PH* player = GamePH::PlayerDI_PH::Get();
			if (!player)
				return true;
			GamePH::InventoryItem* weaponItem = player->GetCurrentWeapon(0);
			if (!weaponItem || !weaponItem->GetItemDescCtx())
				return true;

			return false;
		}

		Tab Tab::instance{};
		void Tab::Update() {
			if (!Menu::menuToggle.GetValue())
				UpdateWeaponDurability(true);
			UpdatePlayerVars();
		}
		void Tab::Render() {
			ImGui::SeparatorText("Current Weapon##Weapon");
			ImGui::BeginDisabled(isWeaponInteractionDisabled() || currentWeaponDurability <= 0.0f); {
				if (ImGui::SliderFloat("Weapon Durability", "Currently only works while your weapon is physically equipped in your hand", &currentWeaponDurability, 0.1f, 999.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
					UpdateWeaponDurability(false);
				else
					UpdateWeaponDurability(true);
				ImGui::EndDisabled();
			}

			ImGui::SeparatorText("Misc##Weapon");
			ImGui::CheckboxHotkey("Unlimited Durability", &unlimitedDurability, "Enables unlimited durability for all weapons");
			ImGui::SameLine();
			ImGui::CheckboxHotkey("Unlimited Ammo", &unlimitedAmmo, "Enables unlimited ammo for all firearms and bows");
			ImGui::CheckboxHotkey("No Spread", &noSpread, "Disables random bullet spread for all firearms and bows (doesn't completely remove spread for bows)");
			ImGui::SameLine();
			ImGui::CheckboxHotkey("No Recoil", &noRecoil, "Disables weapon recoil for all firearms");
			ImGui::CheckboxHotkey("Instant Reload", &instantReload, "Makes reloading firearms and bows (almost) instant");
		}
	}
}