#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\GamePH\PlayerControllerQuery.h>
#include <EGSDK\GamePH\PlayerInfectionModule.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Utils\Memory.h>

namespace EGSDK::GamePH {
	static PlayerInfectionModule* pPlayerInfectionModule = nullptr;
	static const void* cachedPlayerInfectionModuleVtable = nullptr;

	static PlayerDI_PH* ReadModuleOwner(PlayerInfectionModule* infectionModule) {
		if (!infectionModule || Utils::Memory::IsBadReadPtr(infectionModule))
			return nullptr;
		PlayerDI_PH** slot = infectionModule->pPlayerDI_PH.getPointer();
		if (!slot || Utils::Memory::IsBadReadPtr(slot))
			return nullptr;
		return *slot;
	}

	PlayerInfectionModule::~PlayerInfectionModule() = default;

	static PlayerInfectionModule* GetOffset_PlayerInfectionModule() {
		if (!pPlayerInfectionModule)
			return nullptr;
		if (!*reinterpret_cast<void**>(pPlayerInfectionModule))
			return nullptr;
		return pPlayerInfectionModule;
	}

	PlayerInfectionModule* PlayerInfectionModule::Get() {
		PlayerDI_PH* player = PlayerDI_PH::Get();
		if (!player)
			return nullptr;

		if (pPlayerInfectionModule) {
			if (ReadModuleOwner(pPlayerInfectionModule) == player) {
				if (ClassHelpers::SafeGetter<PlayerInfectionModule>(GetOffset_PlayerInfectionModule, false, false))
					return pPlayerInfectionModule;
			}
			pPlayerInfectionModule = nullptr;
		}

		void* found = PlayerControllerQuery::TryFindControllerByRtti(player, "PlayerInfectionModule", &cachedPlayerInfectionModuleVtable);
		if (!found)
			return nullptr;
		pPlayerInfectionModule = reinterpret_cast<PlayerInfectionModule*>(found);
		return ClassHelpers::SafeGetter<PlayerInfectionModule>(GetOffset_PlayerInfectionModule, false, false);
	}

	void PlayerInfectionModule::UpdateClassAddr() {
		PlayerDI_PH* player = PlayerDI_PH::Get();
		if (!player || !pPlayerInfectionModule)
			return;
		if (ReadModuleOwner(pPlayerInfectionModule) != player)
			pPlayerInfectionModule = nullptr;
	}
}
