#pragma once
#include "..\core.h"
#include "menu.h"

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
		extern float tpHorizontalDistanceFromPlayer;

		extern float lensDistortion;
		extern KeyBindOption disablePhotoModeLimits;
		extern KeyBindOption disableSafezoneFOVReduction;
		extern KeyBindOption disableHeadCorrection;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Camera", 1) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}