#include <pch.h>
#include "..\offsets.h"
#include "PlayerState.h"

namespace GamePH {
	PlayerState* PlayerState::Get() {
		__try {
			if (!Offsets::Get_PlayerState())
				return nullptr;

			PlayerState* ptr = *reinterpret_cast<PlayerState**>(Offsets::Get_PlayerState());
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_PlayerState())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}