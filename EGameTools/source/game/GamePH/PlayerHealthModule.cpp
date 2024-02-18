#include <pch.h>
#include "PlayerHealthModule.h"

namespace GamePH {
	PlayerHealthModule* PlayerHealthModule::pPlayerHealthModule = nullptr;
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
}