#include <pch.h>
#include "..\offsets.h"
#include "CGSObject.h"
#include "CLevel.h"

namespace Engine {
	CGSObject* CGSObject::Get() {
		__try {
			CLevel* pCLevel = CLevel::Get();
			if (!pCLevel)
				return nullptr;

			CGSObject* ptr = pCLevel->pCGSObject;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_CGSObject())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}