#include <algorithm>
#include <cmath>
#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex_animation.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\Menu\Camera.h>

namespace EGT::GamePH::Camera {
	void UpdateCameraZoomFOVRuntime() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;
		auto* viewCam = iLevel->GetViewCamera();
		if (!viewCam)
			return;

		static float previousFirstPersonFOV = Menu::Camera::firstPersonFOV;
		static bool hasChangedZoomLevel = false;
		static int zoomLevel = 1;
		static int initialZoomLevel = 1;

		if (!Menu::Camera::thirdPersonCamera.GetValue() && !Menu::Camera::freeCam.GetValue()) {
			constexpr float level1Min = 42.0f;
			constexpr float level2Min = 25.0f;
			constexpr float level3Min = 15.0f;

			if (Menu::Camera::firstPersonZoomIn.IsKeyDown()) {
				if (Menu::Camera::firstPersonZoomIn.IsKeyPressed()) {
					hasChangedZoomLevel = true;
					if (!isZoomingInState) {
						Menu::Camera::originalFirstPersonFOVBeforeZoomIn = std::roundf(viewCam->GetFOV());
						previousFirstPersonFOV = Menu::Camera::originalFirstPersonFOVBeforeZoomIn;
					} else {
						previousFirstPersonFOV = Menu::Camera::firstPersonFOV;
					}

					if (Menu::Camera::originalFirstPersonFOVBeforeZoomIn < level2Min || std::abs(Menu::Camera::originalFirstPersonFOVBeforeZoomIn - level2Min) < 10.0f) {
						zoomLevel = 3;
					} else if (Menu::Camera::originalFirstPersonFOVBeforeZoomIn < level1Min || std::abs(Menu::Camera::originalFirstPersonFOVBeforeZoomIn - level1Min) < 10.0f) {
						zoomLevel = 2;
					} else {
						zoomLevel = 1;
					}

					initialZoomLevel = zoomLevel;
				}

				isZoomingInState = true;

				float targetFOV = previousFirstPersonFOV;
				if (zoomLevel == 1)
					targetFOV = std::max(Menu::Camera::originalFirstPersonFOVBeforeZoomIn - 25.0f, level1Min);
				else if (zoomLevel == 2)
					targetFOV = std::max(Menu::Camera::originalFirstPersonFOVBeforeZoomIn - 45.0f, level2Min);
				else if (zoomLevel == 3)
					targetFOV = std::max(Menu::Camera::originalFirstPersonFOVBeforeZoomIn - 65.0f, level3Min);

				Menu::Camera::firstPersonFOV = ImGui::AnimateLerp("zoomInFOVLerp", previousFirstPersonFOV, targetFOV, 0.3f, hasChangedZoomLevel, &ImGui::AnimEaseOutSine);
				viewCam->SetFOV(Menu::Camera::firstPersonFOV);
				hasChangedZoomLevel = false;

				if (ImGui::KeyBindOption::scrolledMouseWheelUp) {
					ImGui::KeyBindOption::scrolledMouseWheelUp = false;
					if (zoomLevel < 3) {
						zoomLevel++;
						previousFirstPersonFOV = Menu::Camera::firstPersonFOV;
						hasChangedZoomLevel = true;
					}
				} else if (ImGui::KeyBindOption::scrolledMouseWheelDown) {
					ImGui::KeyBindOption::scrolledMouseWheelDown = false;
					if (zoomLevel > initialZoomLevel) {
						zoomLevel--;
						previousFirstPersonFOV = Menu::Camera::firstPersonFOV;
						hasChangedZoomLevel = true;
					}
				}
			} else {
				zoomLevel = 1;
				initialZoomLevel = 1;
				if (Menu::Camera::firstPersonZoomIn.IsKeyReleased()) {
					hasChangedZoomLevel = true;
					previousFirstPersonFOV = Menu::Camera::firstPersonFOV;
				}

				if (!EGSDK::Utils::Values::are_samef(Menu::Camera::firstPersonFOV, Menu::Camera::originalFirstPersonFOVBeforeZoomIn) && isZoomingInState) {
					Menu::Camera::firstPersonFOV = ImGui::AnimateLerp("zoomInFOVLerp", previousFirstPersonFOV, Menu::Camera::originalFirstPersonFOVBeforeZoomIn, 0.25f, hasChangedZoomLevel, &ImGui::AnimEaseOutSine);
					viewCam->SetFOV(Menu::Camera::firstPersonFOV);
					hasChangedZoomLevel = false;
				} else {
					isZoomingInState = false;
				}
			}
		}
	}
}
