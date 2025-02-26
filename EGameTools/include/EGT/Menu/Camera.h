#pragma once
#include <EGSDK\vec3.h>
#include <EGT\Core\Core.h>
#include <EGT\Menu\Menu.h>

namespace EGT::Menu {
	namespace Camera {
		extern EGSDK::vec3 cameraOffset;
		extern float firstPersonFOV;
		extern float originalFirstPersonFOVBeforeZoomIn;
		extern ImGui::KeyBindOption firstPersonZoomIn;
		
		extern ImGui::Option photoMode;

		extern ImGui::KeyBindOption freeCam;
		extern float freeCamFOV;
		extern float freeCamSpeed;
		extern ImGui::KeyBindOption teleportPlayerToCamera;

		extern ImGui::KeyBindOption thirdPersonCamera;
		extern ImGui::KeyBindOption tpUseTPPModel;
		extern float thirdPersonFOV;
		extern float thirdPersonDistanceBehindPlayer;
		extern float thirdPersonHeightAbovePlayer;
		extern float thirdPersonHorizontalDistanceFromPlayer;

		extern float lensDistortion;
		extern ImGui::KeyBindOption goProMode;
		extern ImGui::KeyBindOption disableSafezoneFOVReduction;
		extern ImGui::KeyBindOption disablePhotoModeLimits;
		extern ImGui::KeyBindOption disableHeadCorrection;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Camera", 2) {}
			void Init() override;
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}