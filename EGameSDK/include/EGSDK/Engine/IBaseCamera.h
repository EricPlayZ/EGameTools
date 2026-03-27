#pragma once
#include <stdint.h>
#include <EGSDK\mtx34.h>

#include <EGSDK\Engine\CRTTI.h>

namespace EGSDK::Engine {
	class EGameSDK_API IBaseCamera : public CRTTIObject {
	public:
		float GetFOV();
		vec3* GetForwardVector(vec3* outForwardVec);
		vec3* GetUpVector(vec3* outUpVec);
		vec3* GetLeftVector(vec3* outLeftVec);
		vec3* GetPosition(vec3* outPos);
		mtx34* GetViewMatrix();
		mtx34* GetInvCameraMatrix();

		void Rotate(float angle, const vec3* axis);
		void SetFOV(float fov);
		void SetPosition(const vec3* pos);
		void SetCameraMatrix(const mtx34* mtx);
		void SetInvCameraMatrix(const mtx34* mtx);

		static bool isSetFOVCalledByEGSDK;
	};
}