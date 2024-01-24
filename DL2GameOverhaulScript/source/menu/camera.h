#pragma once
#include "..\core.h"

namespace Menu {
	namespace Camera {
		extern int FOV;
		
		extern Option photoMode;

		extern KeyBindOption freeCam;
		extern float freeCamSpeed;
		extern KeyBindOption teleportPlayerToCamera;

		extern KeyBindOption thirdPersonCamera;
		extern KeyBindOption tpUseTPPModel;
		extern float tpDistanceBehindPlayer;
		extern float tpHeightAbovePlayer;

		extern Option disablePhotoModeLimits;
		extern Option disableSafezoneFOVReduction;

		extern void Update();
		extern void Render();
	}
}