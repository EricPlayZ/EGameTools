#pragma once
#include <vector>
#include <EGSDK\vec3.h>
#include <EGT\Core\Core.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Config\ConfigValue.h>

namespace EGT::Menu {
	namespace Camera {
		struct DollyKeyframe {
			float time = 0.0f;
			vec3 position{};
			vec3 rotation{};
			float fov = 0.0f;
		};

		extern vec3 cameraOffset;
		extern float firstPersonFOV;
		extern float originalFirstPersonFOVBeforeZoomIn;
		extern ImGui::KeyBindOption firstPersonZoomIn;
		
		extern ImGui::Option photoMode;

		extern ImGui::KeyBindOption freeCam;
		extern Config::ConfigFloat freeCamFOV;
		extern Config::ConfigFloat freeCamSpeed;
		extern ImGui::KeyBindOption teleportPlayerToCamera;

		extern ImGui::KeyBindOption thirdPersonCamera;
		extern ImGui::KeyBindOption tpUseTPPModel;
		extern Config::ConfigFloat thirdPersonFOV;
		extern Config::ConfigFloat thirdPersonDistanceBehindPlayer;
		extern Config::ConfigFloat thirdPersonHeightAbovePlayer;
		extern Config::ConfigFloat thirdPersonHorizontalDistanceFromPlayer;

		extern Config::ConfigFloat lensDistortion;
		extern float altLensDistortion;
		extern ImGui::KeyBindOption goProMode;
		extern ImGui::KeyBindOption disableSafezoneFOVReduction;
		extern ImGui::KeyBindOption disablePhotoModeLimits;
		extern ImGui::KeyBindOption disableHeadCorrection;

		extern ImGui::KeyBindOption dollyCamEnabled;
		extern ImGui::KeyBindOption dollyRecording;
		extern ImGui::KeyBindOption dollyPlayPause;
		extern ImGui::KeyBindOption dollyAddKeyframe;
		extern ImGui::KeyBindOption dollyPrevKeyframe;
		extern ImGui::KeyBindOption dollyNextKeyframe;
		extern ImGui::KeyBindOption dollyClearPath;

		extern ImGui::Option dollyLoop;
		extern ImGui::Option dollyPlaying;

		extern Config::ConfigFloat dollyPlaybackSpeed;
		extern Config::ConfigFloat dollyRecordMinInterval;
		extern Config::ConfigFloat dollyRecordPositionThreshold;
		extern Config::ConfigFloat dollyRecordRotationThreshold;
		extern Config::ConfigFloat dollyRecordFOVThreshold;
		extern Config::ConfigFloat dollyDefaultKeyframeInterval;
		extern ImGui::Option dollyPauseRecordingTimeWhenIdle;
		extern ImGui::Option dollyDrawPath;
		extern ImGui::Option dollyDrawInterpolatedPath;
		extern Config::ConfigInt dollyTimingModel;
		extern Config::ConfigFloat dollyTurnSmoothingAggressiveness;
		extern ImGui::Option dollyTurnSmoothingDeviateKeyframes;
		extern Config::ConfigFloat dollyCornerTangentScale;
		extern Config::ConfigFloat dollyOverlayVisualDensity;
		extern Config::ConfigInt dollyOverlaySampleBudget;
		extern Config::ConfigInt dollyInterpolationCurve;
		extern float dollyTimelineTime;
		extern float dollyDuration;
		extern int dollySelectedKeyframe;
		extern bool dollyClearPathConfirm;
		extern std::vector<DollyKeyframe> dollyKeyframes;
		void RenderDollyPathOverlay();

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