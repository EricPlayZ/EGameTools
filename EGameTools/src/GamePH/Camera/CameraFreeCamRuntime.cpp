#include <algorithm>
#include <ImGui\imgui.h>
#include <ImGui\imgui_hotkey.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\GamePH\GameDI_PH.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\Menu\Camera.h>

namespace EGT::GamePH::Camera {
	void UpdateFreeCamRuntime() {
		if (Menu::Camera::photoMode.GetValue())
			return;
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;

		auto* viewCam = iLevel->GetViewCamera();
		if (!viewCam)
			return;
		auto* pGameDI_PH = EGSDK::GamePH::GameDI_PH::Get();
		if (!pGameDI_PH)
			return;
		auto* pFreeCam = EGSDK::GamePH::FreeCamera::Get();
		if (!pFreeCam)
			return;

		static bool prevFreeCam = Menu::Camera::freeCam.GetValue();
		static bool prevEnableSpeedMultiplier = pFreeCam->enableSpeedMultiplier1;
		static float prevSpeedMultiplier = pFreeCam->speedMultiplier;
		static float prevFOV = pFreeCam->GetFOV();

		if (Menu::Camera::freeCam.GetValue() && !iLevel->IsTimerFrozen()) {
			if (viewCam == pFreeCam) {
				pFreeCam->enableSpeedMultiplier1 = true;

				if (ImGui::KeyBindOption::scrolledMouseWheelUp) {
					ImGui::KeyBindOption::scrolledMouseWheelUp = false;
					Menu::Camera::freeCamSpeed = std::min(Menu::Camera::freeCamSpeed + 0.1f, 200.0f);
				} else if (ImGui::KeyBindOption::scrolledMouseWheelDown) {
					ImGui::KeyBindOption::scrolledMouseWheelDown = false;
					Menu::Camera::freeCamSpeed = std::max(Menu::Camera::freeCamSpeed - 0.1f, 0.1f);
				}

				pFreeCam->speedMultiplier = Menu::Camera::freeCamSpeed;
				pFreeCam->SetFOV(Menu::Camera::freeCamFOV);

				if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
					pFreeCam->speedMultiplier *= 2.0f;
				else if (ImGui::IsKeyDown(ImGuiKey_LeftAlt))
					pFreeCam->speedMultiplier /= 2.0f;
				return;
			}

			prevEnableSpeedMultiplier = pFreeCam->enableSpeedMultiplier1;
			prevSpeedMultiplier = pFreeCam->speedMultiplier;
			prevFOV = pFreeCam->GetFOV();

			pGameDI_PH->TogglePhotoMode();
			pFreeCam->AllowCameraMovement(2);
		} else {
			if (prevFreeCam) {
				pFreeCam->enableSpeedMultiplier1 = prevEnableSpeedMultiplier;
				pFreeCam->speedMultiplier = prevSpeedMultiplier;
				pFreeCam->SetFOV(prevFOV);
			}
			if (viewCam != pFreeCam)
				return;

			pGameDI_PH->TogglePhotoMode();
			pFreeCam->AllowCameraMovement(0);
		}

		prevFreeCam = Menu::Camera::freeCam.GetValue();
	}
}
