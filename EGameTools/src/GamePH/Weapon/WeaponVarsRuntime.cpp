#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGT\GamePH\Weapon\WeaponRuntime_Internal.h>
#include <EGT\Menu\Weapon.h>

namespace EGT::GamePH::Weapon {
	static constexpr float baseWeaponDurabilityMul = 1.0f;
	static constexpr float baseWeaponAccuracyMul = 1.0f;
	static constexpr float baseBowAccuracyMul = 0.1f;
	static constexpr float baseWeaponRecoilMul = 1.0f;
	static constexpr float baseWeaponReloadMul = 1.0f;

	void UpdateWeaponVarsRuntime() {
		if (!EGSDK::GamePH::PlayerVariables::gotPlayerVars)
			return;

		EGSDK::GamePH::PlayerVariables::ManageVarByBool("MeleeWpnDurabilityMulReduce", 0.0f, baseWeaponDurabilityMul, Menu::Weapon::unlimitedDurability.GetValue(), true);

		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsPistolInfiniteAmmo", true, false, Menu::Weapon::unlimitedAmmo.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRevolverInfiniteAmmo", true, false, Menu::Weapon::unlimitedAmmo.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRifleInfiniteAmmo", true, false, Menu::Weapon::unlimitedAmmo.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsShotgunInfiniteAmmo", true, false, Menu::Weapon::unlimitedAmmo.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsSMGInfiniteAmmo", true, false, Menu::Weapon::unlimitedAmmo.GetValue());
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("InfiniteArrows", true, false, Menu::Weapon::unlimitedAmmo.GetValue());

		EGSDK::GamePH::PlayerVariables::ManageVarByBool("BulletAccuracyFactor", 0.0f, baseWeaponAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsMoveAccuracyReduce", 0.0f, baseWeaponAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsPistolAccuracyFactor", 0.0f, baseWeaponAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRevolverAccuracyFactor", 0.0f, baseWeaponAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRifleAccuracyFactor", 0.0f, baseWeaponAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsShotgunAccuracyFactor", 0.0f, baseWeaponAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsCrossbowAccuracyFactor", 0.0f, baseBowAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsHarpoonAccuracyFactor", 0.0f, baseBowAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("BowAccuracyFactor", 0.0f, baseBowAccuracyMul, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("BowMaxThrowFactor", 99999.0f, 1.0f, Menu::Weapon::noSpread.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("BowSlowMoAccuracyMul", 0.0f, 0.25f, Menu::Weapon::noSpread.GetValue(), true);

		EGSDK::GamePH::PlayerVariables::ManageVarByBool("BulletRecoilFactor", 0.0f, baseWeaponRecoilMul, Menu::Weapon::noRecoil.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsPistolRecoilFactor", 0.0f, baseWeaponRecoilMul, Menu::Weapon::noRecoil.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRevolverRecoilFactor", 0.0f, baseWeaponRecoilMul, Menu::Weapon::noRecoil.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRifleRecoilFactor", 0.0f, baseWeaponRecoilMul, Menu::Weapon::noRecoil.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsShotgunRecoilFactor", 0.0f, baseWeaponRecoilMul, Menu::Weapon::noRecoil.GetValue(), true);

		EGSDK::GamePH::PlayerVariables::ManageVarByBool("BulletReloadSpeed", 1000.0f, 0.0f, Menu::Weapon::instantReload.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsPistolReloadTimeMul", 1000.0f, baseWeaponReloadMul, Menu::Weapon::instantReload.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRevolverReloadTimeMul", 1000.0f, baseWeaponReloadMul, Menu::Weapon::instantReload.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsRifleReloadTimeMul", 1000.0f, baseWeaponReloadMul, Menu::Weapon::instantReload.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("FirearmsShotgunReloadTimeMul", 1000.0f, baseWeaponReloadMul, Menu::Weapon::instantReload.GetValue(), true);
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("BowPutArrowDuration", 0.0f, 0.137f, Menu::Weapon::instantReload.GetValue(), true);
	}
}
