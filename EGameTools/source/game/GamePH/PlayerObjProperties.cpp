#include <pch.h>
#include "..\offsets.h"
#include "PlayerObjProperties.h"

namespace GamePH {
	PlayerObjProperties* PlayerObjProperties::Get() {
		__try {
			if (!Offsets::Get_g_PlayerObjProperties())
				return nullptr;

			PlayerObjProperties* ptr = *reinterpret_cast<PlayerObjProperties**>(Offsets::Get_g_PlayerObjProperties());
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}