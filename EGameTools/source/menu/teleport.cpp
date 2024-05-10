#include <pch.h>
#include "..\game\Engine\CBulletPhysicsCharacter.h"
#include "..\game\GamePH\FreeCamera.h"
#include "..\game\GamePH\LevelDI.h"
#include "camera.h"
#include "player.h"
#include "teleport.h"

namespace Menu {
	namespace Teleport {
		std::vector<TeleportLocation> savedTeleportLocations;
		static std::vector<const char*> savedTeleportLocationNames;
		static int selectedTPLocation = -1;
		static char newLocationName[25]{};

		static Vector3 teleportPos{};

		static bool isTeleportationDisabled() {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return true;
			if (!Camera::freeCam.GetValue() && !Engine::CBulletPhysicsCharacter::Get())
				return true;
			else if (Camera::freeCam.GetValue() && !GamePH::FreeCamera::Get())
				return true;

			return false;
		}

		static void SyncTPCoordsToPlayer() {
			if (isTeleportationDisabled())
				return;

			if (Camera::freeCam.GetValue()) {
				GamePH::FreeCamera* freeCam = GamePH::FreeCamera::Get();
				if (!freeCam)
					return;

				Vector3 camPos{};
				freeCam->GetPosition(&camPos);
				if (camPos.isDefault())
					return;

				teleportPos = camPos;
			} else {
				Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();
				if (!playerCharacter)
					return;

				teleportPos = playerCharacter->playerPos;
			}
		}
		static void TeleportPlayerTo(const Vector3& pos) {
			if (isTeleportationDisabled())
				return;

			Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();
			
			if (Player::freezePlayer.GetValue()) {
				if (!playerCharacter)
					return;

				playerCharacter->posBeforeFreeze = pos;
				playerCharacter->MoveCharacter(pos);
			} else if (Camera::freeCam.GetValue()) {
				GamePH::FreeCamera* freeCam = GamePH::FreeCamera::Get();
				if (!freeCam)
					return;

				Vector3 camPos{};
				freeCam->GetPosition(&camPos);
				if (camPos.isDefault())
					return;

				// need to implement camera teleportation here :(
			} else {
				if (!playerCharacter)
					return;

				playerCharacter->MoveCharacter(pos);
			}
		}
		static void UpdateTeleportPos() {
			if (isTeleportationDisabled()) {
				if (!teleportPos.isDefault())
					teleportPos = Vector3();
				return;
			}
			if (!teleportPos.isDefault())
				return;

			if (Camera::freeCam.GetValue()) {
				GamePH::FreeCamera* freeCam = GamePH::FreeCamera::Get();
				if (!freeCam)
					return;

				Vector3 camPos{};
				freeCam->GetPosition(&camPos);
				if (camPos.isDefault())
					return;

				teleportPos = camPos;
			} else {
				Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();
				if (!playerCharacter)
					return;

				teleportPos = playerCharacter->playerPos;
			}
		}

		static void SaveTeleportLocation(const char* locationName) {
			if (isTeleportationDisabled()) {
				ImGui::OpenPopup("Couldn't add location");
				return;
			}

			auto savedLocIt = std::find_if(savedTeleportLocations.begin(), savedTeleportLocations.end(), [&locationName](const auto& loc) {
				return loc.name == locationName;
			});
			if (savedLocIt != savedTeleportLocations.end()) {
				ImGui::OpenPopup("The location you have entered already exists, if you want to change it then please remove it and add it again.");
				return;
			}

			Vector3 playerPos{};

			if (Camera::freeCam.GetValue()) {
				GamePH::FreeCamera* freeCam = GamePH::FreeCamera::Get();
				if (!freeCam) {
					ImGui::OpenPopup("Couldn't add location");
					return;
				}

				Vector3 camPos{};
				freeCam->GetPosition(&camPos);
				if (camPos.isDefault()) {
					ImGui::OpenPopup("Couldn't add location");
					return;
				}

				playerPos = camPos;
			} else {
				Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();
				if (!playerCharacter) {
					ImGui::OpenPopup("Couldn't add location");
					return;
				}

				playerPos = playerCharacter->playerPos;
			}
			playerPos = playerPos.round();

			if (savedLocIt != savedTeleportLocations.end() && savedLocIt->pos == playerPos) {
				ImGui::OpenPopup("The location you have entered already exists. Either the name of the location, or the position of the location is already inside the list. If you want to change it then please remove it and add it again.");
				return;
			}

			TeleportLocation tpLocation = TeleportLocation(locationName, playerPos);

			savedTeleportLocations.push_back(tpLocation);
			savedTeleportLocationNames.push_back(locationName);
		}
		static void HandleDialogs() {
			if (ImGui::BeginPopupModal("Give the location a name", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				if (ImGui::InputTextWithHint("##TPLocationNameInputText", "Location name", newLocationName, IM_ARRAYSIZE(newLocationName), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
					SaveTeleportLocation(newLocationName);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopupModal("Location already exists", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("The location you have entered already exists, if you want to change it then please remove it and add it again.");
				if (ImGui::Button("OK", ImVec2(120.0f, 0.0f)))
					ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopupModal("Couldn't add location", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Something went wrong trying to add a location. Either the player class or camera class are not found, or you're in a place in the game where the character or camera isn't properly loaded. If this happens, even though you know it should work fine, please contact @EricPlayZ on NexusMods, GitHub or Discord.");
				if (ImGui::Button("OK", ImVec2(120.0f, 0.0f)))
					ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}
		}

		Tab Tab::instance{};
		void Tab::Update() {
			UpdateTeleportPos();
		}
		void Tab::Render() {
			ImGui::SeparatorText("Saved Locations##Teleport");
			ImGui::BeginDisabled(isTeleportationDisabled()); {
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::ListBox("##SavedTPLocationsListBox", &selectedTPLocation, savedTeleportLocationNames.data(), static_cast<int>(savedTeleportLocationNames.size()), 5);
				ImGui::PopItemWidth();

				ImGui::BeginDisabled(selectedTPLocation < 0 || selectedTPLocation >= savedTeleportLocations.size()); {
					if (ImGui::Button("Teleport to Selected Location"))
						TeleportPlayerTo(savedTeleportLocations[selectedTPLocation].pos);
					ImGui::SameLine();
					if (ImGui::Button("Remove Selected Location")) {
						savedTeleportLocations.erase(savedTeleportLocations.begin() + selectedTPLocation);
						savedTeleportLocationNames.erase(savedTeleportLocationNames.begin() + selectedTPLocation);
						selectedTPLocation = -1;
					}
					ImGui::EndDisabled();
				}
				ImGui::SameLine();
				if (ImGui::Button("Save Current Location"))
					ImGui::OpenPopup("Give the location a name");

				ImGui::EndDisabled();
			}

			ImGui::SeparatorText("Custom##Teleport");
			ImGui::BeginDisabled(isTeleportationDisabled()); {
				static std::string playerPos = "Player Position = X: 0.00, Y: 0.00, Z: 0.00";
				Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();
				if (!playerCharacter)
					playerPos = "Player Position = X: 0.00, Y: 0.00, Z: 0.00";
				else {
					playerPos = "Player Position = X: " + std::format("{:.2f}", playerCharacter->playerPos.data.X) + ", Y: " + std::format("{:.2f}", playerCharacter->playerPos.data.Y) + ", Z: " + std::format("{:.2f}", playerCharacter->playerPos.data.Z);
				}

				static std::string cameraPos = "Free Camera Position = X: 0.00, Y: 0.00, Z: 0.00";
				GamePH::FreeCamera* freeCam = GamePH::FreeCamera::Get();
				if (!Camera::freeCam.GetValue() || !freeCam)
					cameraPos = "Free Camera Position = X: 0.00, Y: 0.00, Z: 0.00";
				else {
					Vector3 camPos{};
					freeCam->GetPosition(&camPos);
					if (camPos.isDefault())
						cameraPos = "Free Camera Position = X: 0.00, Y: 0.00, Z: 0.00";
					else
						cameraPos = "Free Camera Position = X: " + std::format("{:.2f}", camPos.X) + ", Y: " + std::format("{:.2f}", camPos.Y) + ", Z: " + std::format("{:.2f}", camPos.Z);
				}

				ImGui::Text(playerPos.data());
				ImGui::Text(cameraPos.data());

				ImGui::PushItemWidth(200.0f);
				ImGui::InputFloat("X", &teleportPos.X, 1.0f, 10.0f, "%.3f");
				ImGui::SameLine();
				ImGui::InputFloat("Y", &teleportPos.Y, 1.0f, 10.0f, "%.3f");
				ImGui::SameLine();
				ImGui::InputFloat("Z", &teleportPos.Z, 1.0f, 10.0f, "%.3f");
				ImGui::PopItemWidth();

				if (ImGui::Button("Teleport to Coords"))
					TeleportPlayerTo(teleportPos);
				ImGui::SameLine();
				if (ImGui::Button("Get Player Coords"))
					SyncTPCoordsToPlayer();

				ImGui::EndDisabled();
			}

			HandleDialogs();
		}
	}
}