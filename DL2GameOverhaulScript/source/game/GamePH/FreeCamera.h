#pragma once
#include "..\Engine\CBaseCamera.h"
#include "..\buffer.h"

namespace GamePH {
	class CoBaseCameraProxy;

	class FreeCamera : public Engine::CBaseCamera {
	public:
		union {
			buffer<0x18, CoBaseCameraProxy*> pCoBaseCameraProxy;
			buffer<0x38, Engine::CBaseCamera*> pCBaseCamera;
			buffer<0x42, bool> enableSpeedMultiplier1;
			buffer<0x43, bool> enableSpeedMultiplier2;
			buffer<0x1CC, float> speedMultiplier;
		};

		void AllowCameraMovement(int mode = 2);

		static FreeCamera* Get();
	};
}