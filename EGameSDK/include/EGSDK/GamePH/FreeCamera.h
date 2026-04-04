#pragma once
#include <EGSDK\Engine\CBaseCamera.h>
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class CoBaseCameraProxy;

	class EGameSDK_API FreeCamera : public Engine::CBaseCamera {
	public:
		union {
			ClassHelpers::StaticBuffer<0x18, CoBaseCameraProxy*> pCoBaseCameraProxy;
			ClassHelpers::StaticBuffer<0x38, Engine::CBaseCamera*> pCBaseCamera;
			DynamicField(FreeCamera, bool, enableSpeedMultiplier1);
			DynamicField(FreeCamera, bool, enableSpeedMultiplier2);
			DynamicField(FreeCamera, float, speedMultiplier);
			//DynamicField(FreeCamera, float, mouseSensitivityMultiplier);
		};

		void AllowCameraMovement(int mode = 2);

		static FreeCamera* Get();
	};
}