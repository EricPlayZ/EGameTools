#include <pch.h>
#include "..\offsets.h"
#include "CInput.h"

namespace Engine {
	DWORD64 CInput::BlockGameInput() {
		return Utils::Memory::CallVT<2, DWORD64>(this);
	}
	void CInput::UnlockGameInput() {
		Utils::Memory::CallVT<1>(this);
	}

	CInput* CInput::Get() {
		__try {
			if (!Offsets::Get_g_CInput())
				return nullptr;

			CInput* ptr = *reinterpret_cast<CInput**>(Offsets::Get_g_CInput());
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}