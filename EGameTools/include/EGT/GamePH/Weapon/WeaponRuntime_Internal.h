#pragma once

namespace EGT::GamePH::Weapon {
	extern void UpdateWeaponVarsRuntime();
	extern void SyncWeaponDurabilityFromGameRuntime();
	extern void UpdateWeaponDurabilityRuntime(bool updateSlider);
	extern bool IsWeaponInteractionDisabledRuntime();
}
