#include "..\ImGui\imgui.h"
#include "..\game_classes.h"
#include "..\core.h"

namespace Menu {
	namespace Camera {
		extern const int BaseFOV = 57;
		int FOV = 57;

		bool freeCamEnabled = false;
		SMART_BOOL disablePhotoModeLimitsEnabled{};

		static void ToggleFreeCam() {
			GamePH::GameDI_PH* pGameDI_PH = GamePH::GameDI_PH::Get();
			GamePH::FreeCamera* pFreeCamera = GamePH::FreeCamera::Get();

			if (!pGameDI_PH)
				return;
			if (!pFreeCamera)
				return;

			GamePH::GameDI_PH::Get()->TogglePhotoMode();
			GamePH::FreeCamera::Get()->AllowCameraMovement(freeCamEnabled ? 2 : 0);
		}

		void Render() {
			if (ImGui::Checkbox("FreeCam", &freeCamEnabled))
				ToggleFreeCam();
			ImGui::SameLine();
			ImGui::BeginDisabled(freeCamEnabled); {
				if (freeCamEnabled)
					disablePhotoModeLimitsEnabled.Change(true);
				else
					disablePhotoModeLimitsEnabled.Restore();

				ImGui::Checkbox("Disable PhotoMode Limits", &disablePhotoModeLimitsEnabled.value);
				ImGui::EndDisabled();
			}

			Engine::CVideoSettings* pCVideoSettings = Engine::CVideoSettings::Get();
			if (ImGui::SliderInt("FOV", &FOV, 20, 160) && pCVideoSettings) {
				pCVideoSettings->ExtraFOV = static_cast<float>(FOV - BaseFOV);
			} else if (pCVideoSettings)
				FOV = static_cast<int>(pCVideoSettings->ExtraFOV) + Menu::Camera::BaseFOV;
		}
	}
}