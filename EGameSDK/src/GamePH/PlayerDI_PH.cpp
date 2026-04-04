#include <EGSDK\Offsets.h>
#include <EGSDK\Engine\CoPhysics.h>
#include <EGSDK\GamePH\InventoryContainerDI.h>
#include <EGSDK\GamePH\InventoryItem.h>
#include <EGSDK\GamePH\LocalClientDI.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Utils\WinMemory.h>

namespace EGSDK::GamePH {
	bool PlayerDI_PH::areRestrictionsEnabledByGame = false;

	Engine::CoPhysics* PlayerDI_PH::GetCoPhysics() const {
		return Utils::Memory::SafeCallFunctionOffset<Engine::CoPhysics*>(OffsetManager::Get_GetCoPhysics, nullptr, this);
	}

	static InventoryItem* GetOffset_CurrentWeapon(PlayerDI_PH* pPlayerDI_PH, uint32_t indexMaybe) {
		return Utils::Memory::SafeCallFunctionOffset<InventoryItem*>(OffsetManager::Get_PlayerGetCurrentWeapon, nullptr, pPlayerDI_PH, indexMaybe);
	}
	InventoryItem* PlayerDI_PH::GetCurrentWeapon(uint32_t indexMaybe) {
		return ClassHelpers::SafeGetter<InventoryItem>(GetOffset_CurrentWeapon, false, true, this, indexMaybe);
	}

	static InventoryContainerDI* GetOffset_InventoryContainerDI(PlayerDI_PH* pPlayerDI_PH) {
		return pPlayerDI_PH->pInventoryContainerDI;
	}
	InventoryContainerDI* PlayerDI_PH::GetInventoryContainer() {
		return ClassHelpers::SafeGetter<InventoryContainerDI>(GetOffset_InventoryContainerDI, false, true, this);
	}

	bool PlayerDI_PH::EnablePlayerRestrictions(uint64_t* flags) {
		return Utils::Memory::SafeCallFunctionOffset<bool>(OffsetManager::Get_EnablePlayerRestrictionsSubFunc, false, this, flags);
	}
	bool PlayerDI_PH::DisablePlayerRestrictions(uint64_t* flags) {
		return Utils::Memory::SafeCallFunctionOffset<bool>(OffsetManager::Get_DisablePlayerRestrictionsSubFunc, false, this, flags);
	}
	bool PlayerDI_PH::HandlePlayerRestrictions() {
		return Utils::Memory::SafeCallFunctionOffset<bool>(OffsetManager::Get_HandlePlayerRestrictions, false, this);
	}

	static PlayerDI_PH* GetOffset_PlayerDI_PH() {
		LocalClientDI* pLocalClientDI = LocalClientDI::Get();
		return pLocalClientDI ? pLocalClientDI->pPlayerDI_PH : nullptr;
	}
	PlayerDI_PH* PlayerDI_PH::Get() {
		return ClassHelpers::SafeGetter<PlayerDI_PH>(GetOffset_PlayerDI_PH, false, false);
	}
}