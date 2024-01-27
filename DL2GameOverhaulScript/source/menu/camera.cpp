#include <Hotkey.h>
#include <ImGuiEx.h>
#include <imgui.h>
#include "..\core.h"
#include "..\game_classes.h"
#include "..\sigscan\offsets.h"
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
			if (menuToggle.GetValue())
				return;

			Engine::CVideoSettings* videoSettings = Engine::CVideoSettings::Get();
			if (!videoSettings)
				return;

			FOV = static_cast<int>(videoSettings->extraFOV) + baseFOV;
		}
		static void FreeCamUpdate() {
			if (photoMode.GetValue())
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

			if (freeCam.GetValue()) {
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

				freeCam.SetPrevValue(true);
			} else {
				if (freeCam.GetPrevValue()) {
					pFreeCam->enableSpeedMultiplier1 = false;
					pFreeCam->speedMultiplier = 0.1f;
				}
				if (viewCam != pFreeCam)
					return;

				pGameDI_PH->TogglePhotoMode();
				pFreeCam->AllowCameraMovement(0);

				freeCam.SetPrevValue(false);
			}
		}

		static void UpdatePlayerVars() {
			if (!GamePH::PlayerVariables::gotPlayerVars)
				return;

			if (disableSafezoneFOVReduction.GetValue()) {
				GamePH::PlayerVariables::ChangePlayerVar("CameraDefaultFOVReduction", 0.0f);
				disableSafezoneFOVReduction.SetPrevValue(true);
			} else if (disableSafezoneFOVReduction.GetPrevValue()) {
				disableSafezoneFOVReduction.SetPrevValue(false);
				GamePH::PlayerVariables::ChangePlayerVar("CameraDefaultFOVReduction", baseSafezoneFOVReduction);
			}
		}

		void Update() {
			UpdateFOVWhileMenuClosed();
			FreeCamUpdate();
			UpdatePlayerVars();
		}

		void Render() {
			ImGui::SeparatorText("Free Camera");
			ImGui::BeginDisabled(photoMode.GetValue(), &freeCam); {
				ImGui::Checkbox("Enabled##FreeCam", &freeCam);
				ImGui::EndDisabled();
			}
			ImGui::Hotkey("##FreeCamToggleKey", freeCam);
			ImGui::SliderFloat("Speed##FreeCam", &freeCamSpeed, 0.0f, 100.0f);
			ImGui::Checkbox("Teleport Player to Camera", &teleportPlayerToCamera);
			ImGui::Hotkey("##TeleportPlayerToCamToggleKey", teleportPlayerToCamera);

			ImGui::SeparatorText("Third Person Camera");
			ImGui::BeginDisabled(freeCam.GetValue() || photoMode.GetValue(), &thirdPersonCamera); {
				ImGui::Checkbox("Enabled##ThirdPerson", &thirdPersonCamera);
				ImGui::EndDisabled();
			}
			ImGui::Hotkey("##ThirdPersonToggleKey", thirdPersonCamera);
			ImGui::BeginDisabled(freeCam.GetValue() || photoMode.GetValue(), &tpUseTPPModel); {
				ImGui::Checkbox("Use Third Person Player (TPP) Model", &tpUseTPPModel);
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
					FOV = static_cast<int>(pCVideoSettings->extraFOV) + baseFOV;
				ImGui::EndDisabled();
			}
			ImGui::Checkbox("Disable Photo Mode Limits", &disablePhotoModeLimits);
			ImGui::Checkbox("Disable Safezone FOV Reduction", &disableSafezoneFOVReduction);
		}
	}
}