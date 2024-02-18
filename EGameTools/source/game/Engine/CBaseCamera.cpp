#include <pch.h>
#include "CBaseCamera.h"

namespace Engine {
	Vector3* CBaseCamera::GetForwardVector(Vector3* outForwardVec) {
		__try {
			Vector3*(*pGetForwardVector)(LPVOID pCBaseCamera, Vector3* outForwardVec) = (decltype(pGetForwardVector))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?GetForwardVector@IBaseCamera@@QEBA?BVvec3@@XZ");
			if (!pGetForwardVector)
				return nullptr;

			return pGetForwardVector(this, outForwardVec);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	Vector3* CBaseCamera::GetUpVector(Vector3* outUpVec) {
		__try {
			Vector3*(*pGetUpVector)(LPVOID pCBaseCamera, Vector3* outUpVec) = (decltype(pGetUpVector))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?GetUpVector@IBaseCamera@@QEBA?BVvec3@@XZ");
			if (!pGetUpVector)
				return nullptr;

			return pGetUpVector(this, outUpVec);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	Vector3* CBaseCamera::GetLeftVector(Vector3* outLeftVec) {
		__try {
			Vector3*(*pGetLeftVector)(LPVOID pCBaseCamera, Vector3* outLeftVec) = (decltype(pGetLeftVector))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?GetLeftVector@IBaseCamera@@QEBA?BVvec3@@XZ");
			if (!pGetLeftVector)
				return nullptr;

			return pGetLeftVector(this, outLeftVec);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	Vector3* CBaseCamera::GetPosition(Vector3* outPos) {
		__try {
			Vector3*(*pGetPosition)(LPVOID pCBaseCamera, Vector3* outPos) = (decltype(pGetPosition))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?GetPosition@IBaseCamera@@UEBA?BVvec3@@XZ");
			if (!pGetPosition)
				return nullptr;

			return pGetPosition(this, outPos);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}