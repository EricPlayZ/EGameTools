#pragma once
#include <EGT\Core\Core.h>
#include <EGT\Menu\Menu.h>

namespace EGT::Menu {
	namespace Weapon {
		extern float currentWeaponDurability;
		extern ImGui::KeyBindOption unlimitedDurability;
		extern ImGui::KeyBindOption unlimitedAmmo;
		extern ImGui::KeyBindOption noSpread;
		extern ImGui::KeyBindOption noRecoil;
		extern ImGui::KeyBindOption instantReload;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Weapon", 1) {}
			void Init() override;
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}