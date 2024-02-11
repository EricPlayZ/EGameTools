#include <pch.h>
#include "LocalClientDI.h"
#include "SessionCooperativeDI.h"

namespace GamePH {
	LocalClientDI* LocalClientDI::Get() {
		__try {
			SessionCooperativeDI* pSessionCooperativeDI = SessionCooperativeDI::Get();
			if (!pSessionCooperativeDI)
				return nullptr;

			LocalClientDI* ptr = pSessionCooperativeDI->pLocalClientDI;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}