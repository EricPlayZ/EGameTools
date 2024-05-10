#include <pch.h>
#include "..\core.h"
#include "..\game\GamePH\LevelDI.h"
#include "..\game\GamePH\PlayerDI_PH.h"
#include "..\game\GamePH\PlayerVariables.h"
#include "menu.h"
#include "weapon.h"

namespace Menu {
	namespace Weapon {
		float currentWeaponDurability = 150.0f;
		KeyBindOption unlimitedDurability{ VK_NONE };
		static constexpr float baseWeaponDurabilityMul = 1.0f;

		static void UpdatePlayerVars() {
			if (!GamePH::PlayerVariables::gotPlayerVars)
				return;

			GamePH::PlayerVariables::ManagePlayerVarOption("MeleeWpnDurabilityMulReduce", 0.0f, baseWeaponDurabilityMul, &unlimitedDurability, true);
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
			ImGui::SeparatorText("Current Weapon");
			ImGui::BeginDisabled(isWeaponInteractionDisabled()); {
				if (ImGui::SliderFloat("Weapon Durability", "Currently only works while your weapon is physically equipped in your hand", &currentWeaponDurability, 0.1f, 999.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
					UpdateWeaponDurability(false);
				else
					UpdateWeaponDurability(true);
				ImGui::EndDisabled();
			}

			ImGui::SeparatorText("Misc");
			ImGui::CheckboxHotkey("Unlimited Durability", &unlimitedDurability, "Enables unlimited durability for all weapons");
		}
	}
}