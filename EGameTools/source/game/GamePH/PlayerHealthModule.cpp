#include <pch.h>
#include "PlayerDI_PH.h"
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
			playerHealthModulePtrList.clear();
			return nullptr;
		}
	}
	void PlayerHealthModule::Set(LPVOID instance) { pPlayerHealthModule = reinterpret_cast<PlayerHealthModule*>(instance); }

	void PlayerHealthModule::UpdateClassAddr() {
		PlayerDI_PH* pPlayerDI_PH = PlayerDI_PH::Get();
		if (!pPlayerDI_PH)
			return;
		if (PlayerHealthModule::Get() && PlayerHealthModule::Get()->pPlayerDI_PH == pPlayerDI_PH)
			return;

		for (auto& pPlayerHealthModule : PlayerHealthModule::playerHealthModulePtrList) {
			if (pPlayerHealthModule->pPlayerDI_PH == pPlayerDI_PH) {
				PlayerHealthModule::Set(pPlayerHealthModule);
				PlayerHealthModule::playerHealthModulePtrList.clear();
				PlayerHealthModule::playerHealthModulePtrList.emplace_back(pPlayerHealthModule);
			}
		}
	}
}