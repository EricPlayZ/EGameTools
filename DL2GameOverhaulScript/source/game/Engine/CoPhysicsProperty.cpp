#include <pch.h>
#include "..\GamePH\PlayerObjProperties.h"
#include "CoPhysicsProperty.h"

namespace Engine {
	CoPhysicsProperty* CoPhysicsProperty::Get() {
		__try {
			GamePH::PlayerObjProperties* pPlayerObjProperties = GamePH::PlayerObjProperties::Get();
			if (!pPlayerObjProperties)
				return nullptr;

			CoPhysicsProperty* ptr = pPlayerObjProperties->pCoPhysicsProperty;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}