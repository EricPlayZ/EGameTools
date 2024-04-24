#include <pch.h>
#include "..\offsets.h"
#include "FreeCamera.h"

namespace GamePH {
	void FreeCamera::AllowCameraMovement(int mode) {
		__try {
			void(*pAllowCameraMovement)(LPVOID pGameDI_PH, int mode) = (decltype(pAllowCameraMovement))Offsets::Get_AllowCameraMovement();
			if (!pAllowCameraMovement)
				return;

			pAllowCameraMovement(this, mode);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return;
		}
	}

	FreeCamera* FreeCamera::Get() {
		__try {
			PDWORD64 pg_FreeCamera = Offsets::Get_g_FreeCamera();
			if (!pg_FreeCamera)
				return nullptr;

			FreeCamera* ptr = reinterpret_cast<FreeCamera*>(*pg_FreeCamera);
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}