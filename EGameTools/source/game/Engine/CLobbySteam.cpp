#include <pch.h>
#include "..\offsets.h"
#include "CLobbySteam.h"

namespace Engine {
	CLobbySteam* CLobbySteam::Get() {
		__try {
			if (!Offsets::Get_CLobbySteam())
				return nullptr;

			CLobbySteam* ptr = *reinterpret_cast<CLobbySteam**>(Offsets::Get_CLobbySteam());
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_CLobbySteam())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}