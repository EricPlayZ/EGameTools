#include <pch.h>
#include "LocalClientDI.h"
#include "gen_TPPModel.h"

namespace GamePH {
	gen_TPPModel* gen_TPPModel::Get() {
		__try {
			LocalClientDI* pLocalClientDI = LocalClientDI::Get();
			if (!pLocalClientDI)
				return nullptr;

			gen_TPPModel* ptr = pLocalClientDI->pgen_TPPModel;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}