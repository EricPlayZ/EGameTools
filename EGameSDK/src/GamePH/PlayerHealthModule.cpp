#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\GamePH\PlayerControllerQuery.h>
#include <EGSDK\GamePH\PlayerHealthModule.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Utils\Memory.h>

namespace EGSDK::GamePH {
	static PlayerHealthModule* pPlayerHealthModule = nullptr;
	static const void* cachedPlayerHealthModuleVtable = nullptr;

	static PlayerDI_PH* ReadModuleOwner(PlayerHealthModule* healthModule) {
		if (!healthModule || Utils::Memory::IsBadReadPtr(healthModule))
			return nullptr;
		PlayerDI_PH** slot = healthModule->pPlayerDI_PH.getPointer();
		if (!slot || Utils::Memory::IsBadReadPtr(slot))
			return nullptr;
		return *slot;
	}

	PlayerHealthModule::~PlayerHealthModule() = default;

	static PlayerHealthModule* GetOffset_PlayerHealthModule() {
		if (!pPlayerHealthModule)
			return nullptr;
		if (!*reinterpret_cast<void**>(pPlayerHealthModule))
			return nullptr;
		return pPlayerHealthModule;
	}

	PlayerHealthModule* PlayerHealthModule::Get() {
		PlayerDI_PH* player = PlayerDI_PH::Get();
		if (!player)
			return nullptr;

		if (pPlayerHealthModule) {
			if (ReadModuleOwner(pPlayerHealthModule) == player) {
				if (ClassHelpers::SafeGetter<PlayerHealthModule>(GetOffset_PlayerHealthModule, false, false))
					return pPlayerHealthModule;
			}
			pPlayerHealthModule = nullptr;
		}

		void* found = PlayerControllerQuery::TryFindControllerByRtti(player, "PlayerHealthModule", &cachedPlayerHealthModuleVtable);
		if (!found)
			return nullptr;
		pPlayerHealthModule = reinterpret_cast<PlayerHealthModule*>(found);
		return ClassHelpers::SafeGetter<PlayerHealthModule>(GetOffset_PlayerHealthModule, false, false);
	}

	void PlayerHealthModule::UpdateClassAddr() {
		PlayerDI_PH* player = PlayerDI_PH::Get();
		if (!player || !pPlayerHealthModule)
			return;
		if (ReadModuleOwner(pPlayerHealthModule) != player)
			pPlayerHealthModule = nullptr;
	}
}
