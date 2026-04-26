#include <EGT\GamePH\Weapon\WeaponRuntime_Internal.h>
#include <EGT\GamePH\Weapon\WeaponRuntime.h>
#include <EGT\Menu\Menu.h>

namespace EGT::GamePH::Weapon {
	void UpdateWeaponDurability(bool updateSlider) {
		UpdateWeaponDurabilityRuntime(updateSlider);
	}

	bool IsWeaponInteractionDisabled() {
		return IsWeaponInteractionDisabledRuntime();
	}

	void UpdateRuntimeState() {
		if (!Menu::menuToggle.GetValue())
			SyncWeaponDurabilityFromGameRuntime();
		UpdateWeaponVarsRuntime();
	}
}
