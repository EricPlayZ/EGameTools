#pragma once
#include "..\core.h"

namespace Menu {
	namespace Camera {
		extern int FOV;
		
		extern SMART_BOOL photoModeEnabled;

		extern SMART_BOOL freeCamEnabled;
		extern float freeCamSpeed;

		extern SMART_BOOL thirdPersonCameraEnabled;
		extern SMART_BOOL tpUseTPPModel;
		extern float tpDistanceBehindPlayer;
		extern float tpHeightAbovePlayer;

		extern SMART_BOOL disablePhotoModeLimitsEnabled;
		extern bool teleportPlayerToCameraEnabled;

		extern SMART_BOOL disableSafezoneFOVReductionEnabled;

		extern void Update();
		extern void Render();
	}
}