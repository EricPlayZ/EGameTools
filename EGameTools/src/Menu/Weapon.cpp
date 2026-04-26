#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex.h>
#include <EGT\GamePH\Weapon\WeaponRuntime.h>
#include <EGT\Menu\Weapon.h>

namespace EGT::Menu {
	namespace Weapon {
		float currentWeaponDurability = 0.0f;
		ImGui::KeyBindOption unlimitedDurability{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "UnlimitedDurabilityToggleKey" }, ImGui::ConfigBindingInfo{ "Weapon:Misc", "UnlimitedDurability" } };
		ImGui::KeyBindOption unlimitedAmmo{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "UnlimitedAmmoToggleKey" }, ImGui::ConfigBindingInfo{ "Weapon:Misc", "UnlimitedAmmo" } };
		ImGui::KeyBindOption noSpread{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "NoSpreadToggleKey" }, ImGui::ConfigBindingInfo{ "Weapon:Misc", "NoSpread" } };
		ImGui::KeyBindOption noRecoil{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "NoRecoilToggleKey" }, ImGui::ConfigBindingInfo{ "Weapon:Misc", "NoRecoil" } };
		ImGui::KeyBindOption instantReload{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "InstantReloadToggleKey" }, ImGui::ConfigBindingInfo{ "Weapon:Misc", "InstantReload" } };

		Tab Tab::instance{};
		void Tab::Init() {}
		void Tab::Update() {
			GamePH::Weapon::UpdateRuntimeState();
		}

		void Tab::Render() {
			ImGui::SeparatorTextSection("Current Weapon##Weapon", false);
			ImGui::BeginDisabled(GamePH::Weapon::IsWeaponInteractionDisabled() || currentWeaponDurability <= 0.0f);
			GamePH::Weapon::UpdateWeaponDurability(!ImGui::SliderFloat("Weapon Durability", "Currently only works while your weapon is physically equipped in your hand", &currentWeaponDurability, 0.1f, 999.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp));
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
