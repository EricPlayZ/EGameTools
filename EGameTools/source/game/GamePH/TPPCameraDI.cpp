#include <pch.h>
#include "..\offsets.h"
#include "CoBaseCameraProxy.h"
#include "FreeCamera.h"
#include "TPPCameraDI.h"

namespace GamePH {
	TPPCameraDI* TPPCameraDI::Get() {
		__try {
			FreeCamera* pFreeCam = FreeCamera::Get();
			if (!pFreeCam)
				return nullptr;

			CoBaseCameraProxy* pCoBaseCameraProxy = pFreeCam->pCoBaseCameraProxy;
			if (!pCoBaseCameraProxy)
				return nullptr;

			TPPCameraDI* ptr = pCoBaseCameraProxy->pTPPCameraDI;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_TPPCameraDI())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}