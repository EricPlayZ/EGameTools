#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\Weapon.h>

namespace EGT::Menu {
	namespace Weapon {
		float currentWeaponDurability = 0.0f;
		ImGui::KeyBindOption unlimitedDurability{ false, VK_NONE };
		ImGui::KeyBindOption unlimitedAmmo{ false, VK_NONE };
		ImGui::KeyBindOption noSpread{ false, VK_NONE };
		ImGui::KeyBindOption noRecoil{ false, VK_NONE };
		ImGui::KeyBindOption instantReload{ false, VK_NONE };

		static constexpr float baseWeaponDurabilityMul = 1.0f;
		static constexpr float baseWeaponAccuracyMul = 1.0f;
		static constexpr float baseBowAccuracyMul = 0.1f;
		static constexpr float baseWeaponRecoilMul = 1.0f;
		static constexpr float baseWeaponReloadMul = 1.0f;

		static void PlayerVarsUpdate() {
			if (!EGSDK::GamePH::PlayerVariables::gotPlayerVars)
				return;

			EGSDK::GamePH::PlayerVariables::ManageVarByBool("MeleeWpnDurabilityMulReduce", 0.0f, baseWeaponDurabilityMul, unlimitedDurability.GetValue(), true);

			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsPistolInfiniteAmmo", true, false, unlimitedAmmo.GetValue());
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRevolverInfiniteAmmo", true, false, unlimitedAmmo.GetValue());
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRifleInfiniteAmmo", true, false, unlimitedAmmo.GetValue());
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsShotgunInfiniteAmmo", true, false, unlimitedAmmo.GetValue());
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsSMGInfiniteAmmo", true, false, unlimitedAmmo.GetValue());
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("InfiniteArrows", true, false, unlimitedAmmo.GetValue());

			EGSDK::GamePH::PlayerVariables::ManageVarByBool("BulletAccuracyFactor", 0.0f, baseWeaponAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsMoveAccuracyReduce", 0.0f, baseWeaponAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsPistolAccuracyFactor", 0.0f, baseWeaponAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRevolverAccuracyFactor", 0.0f, baseWeaponAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRifleAccuracyFactor", 0.0f, baseWeaponAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsShotgunAccuracyFactor", 0.0f, baseWeaponAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsCrossbowAccuracyFactor", 0.0f, baseBowAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsHarpoonAccuracyFactor", 0.0f, baseBowAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("BowAccuracyFactor", 0.0f, baseBowAccuracyMul, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("BowMaxThrowFactor", 99999.0f, 1.0f, noSpread.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("BowSlowMoAccuracyMul", 0.0f, 0.25f, noSpread.GetValue(), true);

			EGSDK::GamePH::PlayerVariables::ManageVarByBool("BulletRecoilFactor", 0.0f, baseWeaponRecoilMul, noRecoil.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsPistolRecoilFactor", 0.0f, baseWeaponRecoilMul, noRecoil.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRevolverRecoilFactor", 0.0f, baseWeaponRecoilMul, noRecoil.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRifleRecoilFactor", 0.0f, baseWeaponRecoilMul, noRecoil.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsShotgunRecoilFactor", 0.0f, baseWeaponRecoilMul, noRecoil.GetValue(), true);

			EGSDK::GamePH::PlayerVariables::ManageVarByBool("BulletReloadSpeed", 1000.0f, 0.0f, instantReload.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsPistolReloadTimeMul", 1000.0f, baseWeaponReloadMul, instantReload.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRevolverReloadTimeMul", 1000.0f, baseWeaponReloadMul, instantReload.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRifleReloadTimeMul", 1000.0f, baseWeaponReloadMul, instantReload.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsShotgunReloadTimeMul", 1000.0f, baseWeaponReloadMul, instantReload.GetValue(), true);
			EGSDK::GamePH::PlayerVariables::ManageVarByBool("BowPutArrowDuration", 0.0f, 0.137f, instantReload.GetValue(), true);
		}
		static void UpdateWeaponDurability(bool updateSlider) {
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return;
			auto player = EGSDK::GamePH::PlayerDI_PH::Get();
			if (!player)
				return;
			auto weaponItem = player->GetCurrentWeapon(0);
			if (!weaponItem)
				return;
			auto weaponItemCtx = weaponItem->GetItemDescCtx();
			if (!weaponItemCtx)
				return;

			updateSlider ? (currentWeaponDurability = weaponItemCtx->weaponDurability) : (weaponItemCtx->weaponDurability = currentWeaponDurability);
		}

		static bool isWeaponInteractionDisabled() {
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return true;
			auto player = EGSDK::GamePH::PlayerDI_PH::Get();
			if (!player)
				return true;
			auto weaponItem = player->GetCurrentWeapon(0);
			if (!weaponItem || !weaponItem->GetItemDescCtx())
				return true;

			return false;
		}

		Tab Tab::instance{};
		void Tab::Init() {}
		void Tab::Update() {
			if (!Menu::menuToggle.GetValue())
				UpdateWeaponDurability(true);
			PlayerVarsUpdate();
		}
		void Tab::Render() {
			ImGui::SeparatorTextSection("Current Weapon##Weapon", false);
			ImGui::BeginDisabled(isWeaponInteractionDisabled() || currentWeaponDurability <= 0.0f);
			UpdateWeaponDurability(!ImGui::SliderFloat("Weapon Durability", "Currently only works while your weapon is physically equipped in your hand", &currentWeaponDurability, 0.1f, 999.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp));
			ImGui::EndDisabled();

			ImGui::SeparatorTextSection("Misc##Weapon");
			ImGui::CheckboxHotkey("Unlimited Durability", &unlimitedDurability, "Enables unlimited durability for all weapons");
			ImGui::CheckboxHotkey("Unlimited Ammo", &unlimitedAmmo, "Enables unlimited ammo for all firearms and bows");
			ImGui::CheckboxHotkey("No Spread", &noSpread, "Disables random bullet spread for all firearms and bows (doesn't completely remove spread for bows)");
			ImGui::CheckboxHotkey("No Recoil", &noRecoil, "Disables weapon recoil for all firearms");
			ImGui::CheckboxHotkey("Instant Reload", &instantReload, "Makes reloading firearms and bows (almost) instant");
		}
	}
}