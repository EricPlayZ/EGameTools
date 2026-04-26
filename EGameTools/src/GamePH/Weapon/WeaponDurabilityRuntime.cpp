#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGT\GamePH\Weapon\WeaponRuntime_Internal.h>
#include <EGT\Menu\Weapon.h>

namespace EGT::GamePH::Weapon {
	void UpdateWeaponDurabilityRuntime(bool updateSlider) {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;
		auto* player = EGSDK::GamePH::PlayerDI_PH::Get();
		if (!player)
			return;
		auto* weaponItem = player->GetCurrentWeapon(0);
		if (!weaponItem)
			return;
		auto* weaponItemCtx = weaponItem->GetItemDescCtx();
		if (!weaponItemCtx)
			return;

		updateSlider ? (Menu::Weapon::currentWeaponDurability = weaponItemCtx->weaponDurability) : (weaponItemCtx->weaponDurability = Menu::Weapon::currentWeaponDurability);
	}

	bool IsWeaponInteractionDisabledRuntime() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return true;
		auto* player = EGSDK::GamePH::PlayerDI_PH::Get();
		if (!player)
			return true;
		auto* weaponItem = player->GetCurrentWeapon(0);
		if (!weaponItem || !weaponItem->GetItemDescCtx())
			return true;

		return false;
	}

	void SyncWeaponDurabilityFromGameRuntime() {
		UpdateWeaponDurabilityRuntime(true);
	}
}
