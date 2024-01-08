#pragma once
#include "..\core.h"

namespace Menu {
	namespace Camera {
		extern const int BaseFOV;
		extern int FOV;
		
		extern bool photoModeEnabled;
		extern SMART_BOOL freeCamEnabled;
		extern float FreeCamSpeed;

		extern bool thirdPersonCameraEnabled;
		extern float DistanceBehindPlayer;
		extern float HeightAbovePlayer;

		extern SMART_BOOL disablePhotoModeLimitsEnabled;
		extern bool teleportPlayerToCameraEnabled;

		extern SMART_BOOL disableSafezoneFOVReductionEnabled;

		extern void Update();
		extern void Render();
	}
}