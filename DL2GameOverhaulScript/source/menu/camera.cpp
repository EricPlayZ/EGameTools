#include <imgui.h>
#include "..\game_classes.h"
#include "..\core.h"
#include "menu.h"

namespace Menu {
	namespace Camera {
		extern const int BaseFOV = 57;
		int FOV = 57;

		bool photoModeEnabled = false;
		SMART_BOOL freeCamEnabled{};
		SMART_BOOL disablePhotoModeLimitsEnabled{};
		bool teleportPlayerToCameraEnabled = false;

		SMART_BOOL disableSafezoneFOVReductionEnabled{};
		static float previousSafezoneFOVReductionVal = -50.0f;

		static void UpdateFOVWhileMenuClosed() {
			if (Menu::isOpen)
				return;

			Engine::CVideoSettings* videoSettings = Engine::CVideoSettings::Get();
			if (!videoSettings)
				return;

			Menu::Camera::FOV = static_cast<int>(videoSettings->ExtraFOV) + Menu::Camera::BaseFOV;
		}
		static void FreeCamUpdate() {
			if (photoModeEnabled)
				return;
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel)
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
				if (viewCam == pFreeCam)
					return;

				pGameDI_PH->TogglePhotoMode();
				pFreeCam->AllowCameraMovement(2);
			} else {
				GamePH::CameraFPPDI* pPlayerCam = GamePH::CameraFPPDI::Get();
				if (!pPlayerCam || viewCam == pPlayerCam)
					return;

				pGameDI_PH->TogglePhotoMode();
				pFreeCam->AllowCameraMovement(0);
			}
		}

		static bool GetCamDisabledFlag() {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel)
				return true;
			LPVOID viewCam = iLevel->GetViewCamera();
			if (!viewCam)
				return true;
			GamePH::FreeCamera* pFreeCam = GamePH::FreeCamera::Get();
			if (!pFreeCam)
				return true;

			GamePH::CameraFPPDI* pPlayerCam = GamePH::CameraFPPDI::Get();
			if (!pPlayerCam)
				return false;

			DWORD64 viewCamVT = *reinterpret_cast<DWORD64*>(viewCam);
			DWORD64 freeCamVT = *reinterpret_cast<DWORD64*>(pFreeCam);
			DWORD64 playerCamVT = *reinterpret_cast<DWORD64*>(pPlayerCam);
			if (viewCamVT != freeCamVT && viewCamVT != playerCamVT)
				return true;
			
			return false;
		}

		static void UpdatePlayerVars() {
			if (disableSafezoneFOVReductionEnabled.value) {
				auto it = std::find_if(GamePH::PlayerVariables::playerVars.begin(), GamePH::PlayerVariables::playerVars.end(), [](const auto& pair) {
					return pair.first == "CameraDefaultFOVReduction";
				});

				if (it != GamePH::PlayerVariables::playerVars.end()) {
					float* value = reinterpret_cast<float*>(it->second.first);

					if (disableSafezoneFOVReductionEnabled.previousValue != disableSafezoneFOVReductionEnabled.value)
						previousSafezoneFOVReductionVal = *value;

					*value = 0.0f;
					*(value + 1) = 0.0f;
				}
				disableSafezoneFOVReductionEnabled.previousValue = true;
			} else if (disableSafezoneFOVReductionEnabled.previousValue != disableSafezoneFOVReductionEnabled.value) {
				disableSafezoneFOVReductionEnabled.previousValue = false;
				auto it = std::find_if(GamePH::PlayerVariables::playerVars.begin(), GamePH::PlayerVariables::playerVars.end(), [](const auto& pair) {
					return pair.first == "CameraDefaultFOVReduction";
				});

				if (it != GamePH::PlayerVariables::playerVars.end()) {
					float* value = reinterpret_cast<float*>(it->second.first);
					*value = previousSafezoneFOVReductionVal;
					*(value + 1) = previousSafezoneFOVReductionVal;
				}
			}
		}

		void Update() {
			if (photoModeEnabled)
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
			ImGui::BeginDisabled(GetCamDisabledFlag()); {
				ImGui::BeginDisabled(photoModeEnabled); {
					ImGui::Checkbox("FreeCam", &freeCamEnabled.value);
					ImGui::EndDisabled();
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(freeCamEnabled.value); {
					ImGui::Checkbox("Disable PhotoMode Limits", &disablePhotoModeLimitsEnabled.value);
					ImGui::EndDisabled();
				}
				ImGui::BeginDisabled(photoModeEnabled); {
					ImGui::Checkbox("Teleport Player to Camera", &teleportPlayerToCameraEnabled);
					ImGui::EndDisabled();
				}
				ImGui::EndDisabled();
			}
			
			Engine::CVideoSettings* pCVideoSettings = Engine::CVideoSettings::Get();
			ImGui::BeginDisabled(!pCVideoSettings); {
				if (ImGui::SliderInt("FOV", &FOV, 20, 160) && pCVideoSettings)
					pCVideoSettings->ExtraFOV = static_cast<float>(FOV - BaseFOV);
				else if (pCVideoSettings)
					FOV = static_cast<int>(pCVideoSettings->ExtraFOV) + Menu::Camera::BaseFOV;
				ImGui::EndDisabled();
			}

			ImGui::BeginDisabled(!GamePH::PlayerVariables::gotPlayerVars); {
				ImGui::Checkbox("Disable Safezone FOV Reduction", &disableSafezoneFOVReductionEnabled.value);
				ImGui::EndDisabled();
			}
		}
	}
}