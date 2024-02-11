#pragma once
#include "..\Vector3.h"

namespace Engine {
	class CBaseCamera {
	public:
		/*union {
			buffer<0x48, float> yaw;
			buffer<0x4C, float> X;
			buffer<0x58, float> pitch;
			buffer<0x5C, float> Y;
			buffer<0x6C, float> Z;
		};*/
		Vector3* GetForwardVector(Vector3* outForwardVec);
		Vector3* GetUpVector(Vector3* outUpVec);
		Vector3* GetLeftVector(Vector3* outLeftVec);
		Vector3* GetPosition(Vector3* outPos);
	};
}