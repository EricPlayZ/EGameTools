#include <pch.h>
#include "..\core.h"
#include "..\game\Engine\CVideoSettings.h"
#include "..\game\Engine\engine_hooks.h"
#include "..\game\GamePH\FreeCamera.h"
#include "..\game\GamePH\GameDI_PH.h"
#include "..\game\GamePH\LevelDI.h"
#include "..\game\GamePH\PlayerVariables.h"
#include "..\game\GamePH\gameph_misc.h"
#include "..\game\GamePH\gen_TPPModel.h"
#include "..\offsets.h"
#include "camera.h"
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
		float tpHorizontalDistanceFromPlayer = 0.0f;

		KeyBindOption disablePhotoModeLimits{ VK_NONE };
		KeyBindOption disableSafezoneFOVReduction{ VK_NONE };

		static constexpr int baseFOV = 57;
		static constexpr float baseSafezoneFOVReduction = -10.0f;

		static void UpdateFOV() {
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

			static bool prevFreeCam = freeCam.GetValue();
			static bool prevEanbleSpeedMultiplier = pFreeCam->enableSpeedMultiplier1;
			static float prevSpeedMultiplier = pFreeCam->speedMultiplier;
			static float prevFOV = pFreeCam->FOV;
			if (freeCam.GetValue() && !iLevel->IsTimerFrozen()) {
				if (viewCam == pFreeCam) {
					pFreeCam->enableSpeedMultiplier1 = true;

					if (KeyBindOption::scrolledMouseWheelUp) {
						KeyBindOption::scrolledMouseWheelUp = false;
						freeCamSpeed += 0.1f;
					} else if (KeyBindOption::scrolledMouseWheelDown) {
						KeyBindOption::scrolledMouseWheelDown = false;
						freeCamSpeed -= 0.1f;
					}

					if (freeCamSpeed < 0.1f)
						freeCamSpeed = 0.1f;
					else if (freeCamSpeed > 200.0f)
						freeCamSpeed = 200.0f;

					pFreeCam->speedMultiplier = freeCamSpeed;
					pFreeCam->FOV = static_cast<float>(FOV);

					if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
						pFreeCam->speedMultiplier *= 2.0f;
					else if (ImGui::IsKeyDown(ImGuiKey_LeftAlt))
						pFreeCam->speedMultiplier /= 2.0f;

					pFreeCam->GetPosition(&Engine::Hooks::freeCamPosBeforeGamePause);

					return;
				}

				prevEanbleSpeedMultiplier = pFreeCam->enableSpeedMultiplier1;
				prevSpeedMultiplier = pFreeCam->speedMultiplier;
				prevFOV = pFreeCam->FOV;

				pGameDI_PH->TogglePhotoMode();
				pFreeCam->AllowCameraMovement(2);
			} else {
				Engine::Hooks::switchedFreeCamByGamePause = freeCam.GetValue() && iLevel->IsTimerFrozen();

				if (prevFreeCam) {
					pFreeCam->enableSpeedMultiplier1 = prevEanbleSpeedMultiplier;
					pFreeCam->speedMultiplier = prevSpeedMultiplier;
					pFreeCam->FOV = prevFOV;
				}
				if (viewCam != pFreeCam)
					return;

				pGameDI_PH->TogglePhotoMode();
				pFreeCam->AllowCameraMovement(0);
			}

			prevFreeCam = freeCam.GetValue();
		}
		static void UpdateTPPModel() {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return;

			GamePH::gen_TPPModel* pgen_TPPModel = GamePH::gen_TPPModel::Get();
			if (pgen_TPPModel) {
				if (Menu::Camera::freeCam.GetValue() && !iLevel->IsTimerFrozen())
					GamePH::ShowTPPModel(true);
				else if (Menu::Camera::freeCam.GetValue() && iLevel->IsTimerFrozen() && !photoMode.GetValue())
					GamePH::ShowTPPModel(false);
				else if (Menu::Camera::thirdPersonCamera.GetValue() && Menu::Camera::tpUseTPPModel.GetValue())
					GamePH::ShowTPPModel(true);
				else if (!photoMode.GetValue())
					GamePH::ShowTPPModel(false);
			}
		}
		static void UpdatePlayerVars() {
			if (!GamePH::PlayerVariables::gotPlayerVars)
				return;

			GamePH::PlayerVariables::ManagePlayerVarOption("CameraDefaultFOVReduction", 0.0f, baseSafezoneFOVReduction, &disableSafezoneFOVReduction);
		}
		static void UpdateDisabledOptions() {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			freeCam.SetChangesAreDisabled(!iLevel || !iLevel->IsLoaded() || photoMode.GetValue());
			teleportPlayerToCamera.SetChangesAreDisabled(!iLevel || !iLevel->IsLoaded());
			thirdPersonCamera.SetChangesAreDisabled(freeCam.GetValue() || photoMode.GetValue());
			tpUseTPPModel.SetChangesAreDisabled(freeCam.GetValue() || photoMode.GetValue());
		}

		Tab Tab::instance{};
		void Tab::Update() {
			UpdateFOV();
			FreeCamUpdate();
			UpdateTPPModel();
			UpdatePlayerVars();
			UpdateDisabledOptions();
		}
		void Tab::Render() {
			ImGui::SeparatorText("Free Camera");
			ImGui::BeginDisabled(freeCam.GetChangesAreDisabled() || photoMode.GetValue()); {
				ImGui::CheckboxHotkey("Enabled##FreeCam", &freeCam, "Enables free camera which allows you to travel anywhere with the camera");
				ImGui::EndDisabled();
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(teleportPlayerToCamera.GetChangesAreDisabled()); {
				ImGui::CheckboxHotkey("Teleport Player to Camera", &teleportPlayerToCamera, "Teleports the player to the camera while Free Camera is activated");
				ImGui::EndDisabled();
			}
			ImGui::SliderFloat("Speed##FreeCam", &freeCamSpeed, 0.1f, 200.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);

			ImGui::SeparatorText("Third Person Camera");
			ImGui::BeginDisabled(thirdPersonCamera.GetChangesAreDisabled()); {
				ImGui::CheckboxHotkey("Enabled##ThirdPerson", &thirdPersonCamera, "Enables the third person camera");
				ImGui::EndDisabled();
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(tpUseTPPModel.GetChangesAreDisabled()); {
				ImGui::CheckboxHotkey("Use Third Person Player (TPP) Model", &tpUseTPPModel, "Uses Aiden's TPP (Third Person Player) model while the third person camera is enabled");
				ImGui::EndDisabled();
			}
			ImGui::SliderFloat("Distance behind player", &tpDistanceBehindPlayer, 1.0f, 10.0f, "%.2fm");
			ImGui::SliderFloat("Height above player", &tpHeightAbovePlayer, 1.0f, 3.0f, "%.2fm");
			ImGui::SliderFloat("Horizontal distance from player", &tpHorizontalDistanceFromPlayer, -2.0f, 2.0f, "%.2fm");

			ImGui::SeparatorText("Misc");
			Engine::CVideoSettings* pCVideoSettings = Engine::CVideoSettings::Get();
			ImGui::BeginDisabled(!pCVideoSettings); {
				if (ImGui::SliderInt("FOV", &FOV, 20, 160) && pCVideoSettings)
					pCVideoSettings->extraFOV = static_cast<float>(FOV - baseFOV);
				else if (pCVideoSettings)
					FOV = static_cast<int>(pCVideoSettings->extraFOV) + baseFOV;
				ImGui::EndDisabled();
			}
			ImGui::CheckboxHotkey("Disable Photo Mode Limits", &disablePhotoModeLimits, "Disables the invisible box while in Photo Mode");
			ImGui::CheckboxHotkey("Disable Safezone FOV Reduction", &disableSafezoneFOVReduction, "Disables the FOV reduction that happens while you're in a safezone");
		}
	}
}