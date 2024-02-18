#include <pch.h>
#include "CGSObject2.h"
#include "CLevel2.h"

namespace Engine {
	CGSObject2* CGSObject2::Get() {
		__try {
			CLevel2* pCLevel2 = CLevel2::Get();
			if (!pCLevel2)
				return nullptr;

			CGSObject2* ptr = pCLevel2->pCGSObject2;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}