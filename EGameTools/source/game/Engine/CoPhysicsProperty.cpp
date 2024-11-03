#include <pch.h>
#include "..\GamePH\PlayerDI_PH.h"
#include "CoPhysicsProperty.h"

namespace Engine {
	CoPhysicsProperty* CoPhysicsProperty::Get() {
		__try {
			GamePH::PlayerDI_PH* pPlayerDI_PH = GamePH::PlayerDI_PH::Get();
			if (!pPlayerDI_PH)
				return nullptr;

			CoPhysicsProperty* ptr = pPlayerDI_PH->pCoPhysicsProperty;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}