#include <pch.h>
#include "LevelDI.h"
#include "PlayerHealthModule.h"

namespace GamePH {
	static PlayerHealthModule* pPlayerHealthModule = nullptr;
	std::vector<PlayerHealthModule*> PlayerHealthModule::playerHealthModulePtrList{};

	PlayerHealthModule* PlayerHealthModule::Get() {
		__try {
			if (!pPlayerHealthModule)
				return nullptr;
			if (!*reinterpret_cast<LPVOID*>(pPlayerHealthModule))
				return nullptr;

			return pPlayerHealthModule;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			pPlayerHealthModule = nullptr;
			return nullptr;
		}
	}
	void PlayerHealthModule::Set(LPVOID instance) { pPlayerHealthModule = reinterpret_cast<PlayerHealthModule*>(instance); }

	void PlayerHealthModule::UpdateClassAddr() {
		LevelDI* iLevel = LevelDI::Get();
		if (!iLevel)
			return;
		if (PlayerHealthModule::Get() && PlayerHealthModule::Get()->pPlayerDI_PH == iLevel->pPlayerDI_PH)
			return;

		for (auto& pPlayerHealthModule : PlayerHealthModule::playerHealthModulePtrList) {
			if (pPlayerHealthModule->pPlayerDI_PH == iLevel->pPlayerDI_PH) {
				PlayerHealthModule::Set(pPlayerHealthModule);
				PlayerHealthModule::playerHealthModulePtrList.clear();
				PlayerHealthModule::playerHealthModulePtrList.emplace_back(pPlayerHealthModule);
			}
		}
	}
}