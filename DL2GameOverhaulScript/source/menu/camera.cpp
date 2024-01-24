#include <imgui.h>
#include <Hotkey.h>
#include "..\sigscan\offsets.h"
#include "..\game_classes.h"
#include "..\core.h"
#include "menu.h"

namespace Menu {
	namespace Camera {
		int FOV = 57;

		Option photoMode{};

		KeyBindOption freeCam{ VK_F3 };
		float freeCamSpeed = 2.0f;
		KeyBindOption teleportPlayerToCamera{ VK_F4 };

		KeyBindOption thirdPersonCamera{ VK_F1 };
		KeyBindOption tpUseTPPModel{ VK_F2 };
		float tpDistanceBehindPlayer = 2.0f;
		float tpHeightAbovePlayer = 1.35f;

		Option disablePhotoModeLimits{};
		Option disableSafezoneFOVReduction{};

		static const int baseFOV = 57;
		static const float baseSafezoneFOVReduction = -10.0f;

		static void UpdateFOVWhileMenuClosed() {
			if (Menu::menuToggle.IsEnabled())
				return;

			Engine::CVideoSettings* videoSettings = Engine::CVideoSettings::Get();
			if (!videoSettings)
				return;

			Menu::Camera::FOV = static_cast<int>(videoSettings->extraFOV) + Menu::Camera::baseFOV;
		}
		static void FreeCamUpdate() {
			if (photoMode.IsEnabled())
				return;
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return;
			LPVOID viewCam = iLevel->GetViewCamera();
			if (!viewCam)
				return;
			GamePH::GameDI_PH* pGameDI_PH = GamePH::GameDI_PH::Get();
			if (!pGameDI_PH)
				return;
			GamePH::FreeCamera* pFreeCam = GamePH::FreeCamera::Get();
			if (!pFreeCam)
				return;

			if (freeCam.IsEnabled()) {
				if (viewCam == pFreeCam) {
					pFreeCam->enableSpeedMultiplier1 = true;
					pFreeCam->speedMultiplier = freeCamSpeed;
					if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
						pFreeCam->speedMultiplier *= 2.0f;
					else if (ImGui::IsKeyDown(ImGuiKey_LeftAlt))
						pFreeCam->speedMultiplier /= 2.0f;

					return;
				}

				pGameDI_PH->TogglePhotoMode();
				pFreeCam->AllowCameraMovement(2);

				GamePH::ShowTPPModel(true);
			} else {
				if (freeCam.WasEnabled()) {
					pFreeCam->enableSpeedMultiplier1 = false;
					pFreeCam->speedMultiplier = 0.1f;
				}
				if (viewCam != pFreeCam)
					return;

				pGameDI_PH->TogglePhotoMode();
				pFreeCam->AllowCameraMovement(0);

				GamePH::ShowTPPModel(false);
			}
		}

		static void UpdatePlayerVars() {
			if (!GamePH::PlayerVariables::gotPlayerVars)
				return;

			if (disableSafezoneFOVReduction.IsEnabled()) {
				GamePH::PlayerVariables::ChangePlayerVar("CameraDefaultFOVReduction", 0.0f);
				disableSafezoneFOVReduction.Change(true);
			} else if (disableSafezoneFOVReduction.WasEnabled()) {
				disableSafezoneFOVReduction.Restore(true);
				GamePH::PlayerVariables::ChangePlayerVar("CameraDefaultFOVReduction", baseSafezoneFOVReduction);
			}
		}

		void Update() {
			if (photoMode.IsEnabled())
				freeCam.Change(false);
			else
				freeCam.Restore();

			if (freeCam.IsEnabled())
				disablePhotoModeLimits.Change(true);
			else
				disablePhotoModeLimits.Restore();

			UpdateFOVWhileMenuClosed();
			FreeCamUpdate();
			UpdatePlayerVars();
		}

		void Render() {
			ImGui::SeparatorText("Free Camera");
			ImGui::BeginDisabled(photoMode.IsEnabled()); {
				ImGui::Checkbox("Enabled##FreeCam", &freeCam.value);
				ImGui::EndDisabled();
			}
			ImGui::Hotkey("##FreeCamToggleKey", freeCam);
			ImGui::SliderFloat("Speed##FreeCam", &freeCamSpeed, 0.0f, 100.0f);
			ImGui::Checkbox("Teleport Player to Camera", &teleportPlayerToCamera.value);
			ImGui::Hotkey("##TeleportPlayerToCamToggleKey", teleportPlayerToCamera);

			ImGui::SeparatorText("Third Person Camera");
			ImGui::BeginDisabled(freeCam.IsEnabled() || photoMode.IsEnabled()); {
				ImGui::Checkbox("Enabled##ThirdPerson", &thirdPersonCamera.value);
				ImGui::EndDisabled();
			}
			ImGui::Hotkey("##ThirdPersonToggleKey", thirdPersonCamera);
			ImGui::BeginDisabled(freeCam.IsEnabled() || photoMode.IsEnabled()); {
				ImGui::Checkbox("Use Third Person Player (TPP) Model", &tpUseTPPModel.value);
				ImGui::EndDisabled();
			}
			ImGui::Hotkey("##TPPModelToggleKey", tpUseTPPModel);
			ImGui::SliderFloat("Distance behind player", &tpDistanceBehindPlayer, 1.0f, 10.0f);
			ImGui::SliderFloat("Height above player", &tpHeightAbovePlayer, 1.0f, 3.0f);

			ImGui::SeparatorText("Misc");
			Engine::CVideoSettings* pCVideoSettings = Engine::CVideoSettings::Get();
			ImGui::BeginDisabled(!pCVideoSettings); {
				if (ImGui::SliderInt("FOV", &FOV, 20, 160) && pCVideoSettings)
					pCVideoSettings->extraFOV = static_cast<float>(FOV - baseFOV);
				else if (pCVideoSettings)
					FOV = static_cast<int>(pCVideoSettings->extraFOV) + Menu::Camera::baseFOV;
				ImGui::EndDisabled();
			}
			ImGui::BeginDisabled(freeCam.IsEnabled()); {
				ImGui::Checkbox("Disable Photo Mode Limits", &disablePhotoModeLimits.value);
				ImGui::EndDisabled();
			}
			ImGui::Checkbox("Disable Safezone FOV Reduction", &disableSafezoneFOVReduction.value);
		}
	}
}