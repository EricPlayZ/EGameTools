#include <imgui.h>
#include "..\sigscan\offsets.h"
#include "..\game_classes.h"
#include "..\core.h"
#include "menu.h"

namespace Menu {
	namespace Camera {
		int FOV = 57;

		SMART_BOOL photoModeEnabled;

		SMART_BOOL freeCamEnabled{};
		KeyBindToggle freeCamToggleKey = KeyBindToggle(KeyBind::F3);
		float freeCamSpeed = 2.0f;
		bool teleportPlayerToCameraEnabled = false;
		KeyBindToggle teleportPlayerToCameraToggleKey = KeyBindToggle(KeyBind::F4);

		SMART_BOOL thirdPersonCameraEnabled;
		KeyBindToggle thirdPersonCameraToggleKey = KeyBindToggle(KeyBind::F1);
		SMART_BOOL tpUseTPPModelEnabled;
		KeyBindToggle tpUseTPPModelToggleKey = KeyBindToggle(KeyBind::F2);
		float tpDistanceBehindPlayer = 2.0f;
		float tpHeightAbovePlayer = 1.35f;

		SMART_BOOL disablePhotoModeLimitsEnabled{};
		SMART_BOOL disableSafezoneFOVReductionEnabled{};

		static const int baseFOV = 57;
		static const float baseSafezoneFOVReduction = -10.0f;
		static LPVOID previousViewCam = nullptr;

		static void UpdateFOVWhileMenuClosed() {
			if (Menu::isOpen)
				return;

			Engine::CVideoSettings* videoSettings = Engine::CVideoSettings::Get();
			if (!videoSettings)
				return;

			Menu::Camera::FOV = static_cast<int>(videoSettings->extraFOV) + Menu::Camera::baseFOV;
		}
		static void FreeCamUpdate() {
			if (photoModeEnabled.value)
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

			if (freeCamEnabled.value) {
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
				if (freeCamEnabled.previousValue) {
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

			if (disableSafezoneFOVReductionEnabled.value) {
				GamePH::PlayerVariables::ChangePlayerVar("CameraDefaultFOVReduction", 0.0f);
				disableSafezoneFOVReductionEnabled.previousValue = true;
			} else if (disableSafezoneFOVReductionEnabled.previousValue != disableSafezoneFOVReductionEnabled.value) {
				disableSafezoneFOVReductionEnabled.previousValue = false;
				GamePH::PlayerVariables::ChangePlayerVar("CameraDefaultFOVReduction", baseSafezoneFOVReduction);
			}
		}

		void Update() {
			if (photoModeEnabled.value)
				freeCamEnabled.Change(false);
			else
				freeCamEnabled.Restore();

			if (freeCamEnabled.value)
				disablePhotoModeLimitsEnabled.Change(true);
			else
				disablePhotoModeLimitsEnabled.Restore();

			UpdateFOVWhileMenuClosed();
			FreeCamUpdate();
			UpdatePlayerVars();
		}

		void Render() {
			ImGui::SeparatorText("Free Camera");
			ImGui::BeginDisabled(photoModeEnabled.value); {
				ImGui::Checkbox("Enabled##FreeCam", &freeCamEnabled.value);
				ImGui::EndDisabled();
			}
			ImGui::Hotkey("##FreeCamToggleKey", freeCamToggleKey);
			ImGui::SliderFloat("Speed##FreeCam", &freeCamSpeed, 0.0f, 100.0f);
			ImGui::Checkbox("Teleport Player to Camera", &teleportPlayerToCameraEnabled);
			ImGui::Hotkey("##TeleportPlayerToCamToggleKey", teleportPlayerToCameraToggleKey);

			ImGui::SeparatorText("Third Person Camera");
			ImGui::BeginDisabled(freeCamEnabled.value || photoModeEnabled.value); {
				ImGui::Checkbox("Enabled##ThirdPerson", &thirdPersonCameraEnabled.value);
				ImGui::EndDisabled();
			}
			ImGui::Hotkey("##ThirdPersonToggleKey", thirdPersonCameraToggleKey);
			ImGui::BeginDisabled(freeCamEnabled.value || photoModeEnabled.value); {
				ImGui::Checkbox("Use Third Person Player (TPP) Model", &tpUseTPPModelEnabled.value);
				ImGui::EndDisabled();
			}
			ImGui::Hotkey("##TPPModelToggleKey", tpUseTPPModelToggleKey);
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
			ImGui::BeginDisabled(freeCamEnabled.value); {
				ImGui::Checkbox("Disable Photo Mode Limits", &disablePhotoModeLimitsEnabled.value);
				ImGui::EndDisabled();
			}
			ImGui::Checkbox("Disable Safezone FOV Reduction", &disableSafezoneFOVReductionEnabled.value);
		}
	}
}