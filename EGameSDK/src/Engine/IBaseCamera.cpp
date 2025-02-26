#include <EGSDK\Engine\IBaseCamera.h>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Utils\WinMemory.h>

namespace EGSDK::Engine {
	bool IBaseCamera::isSetFOVCalledByEGSDK = false;

	float IBaseCamera::GetFOV() {
		return Utils::Memory::SafeCallFunction<float>("engine_x64_rwdi.dll", "?GetFOV@IBaseCamera@@QEBAMXZ", -1.0f, this);
	}
	vec3* IBaseCamera::GetForwardVector(vec3* outForwardVec) {
		return Utils::Memory::SafeCallFunction<vec3*>("engine_x64_rwdi.dll", "?GetForwardVector@IBaseCamera@@QEBA?BVvec3@@XZ", nullptr, this, outForwardVec);
	}
	vec3* IBaseCamera::GetUpVector(vec3* outUpVec) {
		return Utils::Memory::SafeCallFunction<vec3*>("engine_x64_rwdi.dll", "?GetUpVector@IBaseCamera@@QEBA?BVvec3@@XZ", nullptr, this, outUpVec);
	}
	vec3* IBaseCamera::GetLeftVector(vec3* outLeftVec) {
		return Utils::Memory::SafeCallFunction<vec3*>("engine_x64_rwdi.dll", "?GetLeftVector@IBaseCamera@@QEBA?BVvec3@@XZ", nullptr, this, outLeftVec);
	}
	vec3* IBaseCamera::GetPosition(vec3* outPos) {
		return Utils::Memory::SafeCallFunction<vec3*>("engine_x64_rwdi.dll", "?GetPosition@IBaseCamera@@UEBA?BVvec3@@XZ", nullptr, this, outPos);
	}
	mtx34* IBaseCamera::GetViewMatrix() {
		return Utils::Memory::SafeCallFunction<mtx34*>("engine_x64_rwdi.dll", "?GetViewMatrix@IBaseCamera@@QEAAAEBVmtx34@@XZ", nullptr, this);
	}
	mtx34* IBaseCamera::GetInvCameraMatrix() {
		return Utils::Memory::SafeCallFunction<mtx34*>("engine_x64_rwdi.dll", "?GetInvCameraMatrix@IBaseCamera@@QEAAAEBVmtx34@@XZ", nullptr, this);
	}

	void IBaseCamera::Rotate(float angle, const vec3* axis) {
		Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?Rotate@IBaseCamera@@QEAAXMAEBVvec3@@@Z", this, angle, axis);
	}
	void IBaseCamera::SetFOV(float fov) {
		isSetFOVCalledByEGSDK = true;
		Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?SetFOV@IBaseCamera@@QEAAXM@Z", this, fov);
		isSetFOVCalledByEGSDK = false;
	}
	void IBaseCamera::SetPosition(const vec3* pos) {
		Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?SetPosition@IBaseCamera@@QEAAXAEBVvec3@@@Z", this, pos);
	}
	void IBaseCamera::SetCameraMatrix(const mtx34* mtx) {
		Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?SetCameraMatrix@IBaseCamera@@QEAAXAEBVmtx34@@@Z", this, mtx);
	}
	void IBaseCamera::SetInvCameraMatrix(const mtx34* mtx) {
		Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?SetInvCameraMatrix@IBaseCamera@@QEAAXAEBVmtx34@@@Z", this, mtx);
	}
}