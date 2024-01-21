#pragma once
#include <Hotkey.h>
#include "..\core.h"

namespace Menu {
	namespace Camera {
		extern int FOV;
		
		extern SMART_BOOL photoModeEnabled;

		extern SMART_BOOL freeCamEnabled;
		extern KeyBindToggle freeCamToggleKey;
		extern float freeCamSpeed;
		extern bool teleportPlayerToCameraEnabled;
		extern KeyBindToggle teleportPlayerToCameraToggleKey;

		extern SMART_BOOL thirdPersonCameraEnabled;
		extern KeyBindToggle thirdPersonCameraToggleKey;
		extern SMART_BOOL tpUseTPPModelEnabled;
		extern KeyBindToggle tpUseTPPModelToggleKey;
		extern float tpDistanceBehindPlayer;
		extern float tpHeightAbovePlayer;

		extern SMART_BOOL disablePhotoModeLimitsEnabled;
		extern SMART_BOOL disableSafezoneFOVReductionEnabled;

		extern void Update();
		extern void Render();
	}
}