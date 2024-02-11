#include <pch.h>
#include "CGSObject.h"
#include "CLevel2.h"

namespace Engine {
	CLevel2* CLevel2::Get() {
		__try {
			CGSObject* pCGSObject = CGSObject::Get();
			if (!pCGSObject)
				return nullptr;

			CLevel2* ptr = pCGSObject->pCLevel2;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}