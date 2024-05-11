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
		std::vector<const char*> savedTeleportLocationNames;
		static int selectedTPLocation = -1;
		static char newLocationName[25]{};

		static Vector3 teleportCoords{};

		KeyBindOption teleportToSelectedLocation{ VK_F9 };
		KeyBindOption teleportToCoords{ VK_NONE };

		std::vector<TeleportLocation> ParseTeleportLocations(const std::string& input) {
			if (input.empty())
				return {};

			std::vector<TeleportLocation> teleportLocations{};
			std::stringstream ss(input);
			std::string item{};

			while (std::getline(ss, item, ';')) {
				size_t colonPos = item.rfind(':');
				if (colonPos != std::string::npos) {
					std::string tpLocName = item.substr(0, colonPos);
					std::string tpLocCoords = item.substr(colonPos + 1);

					std::stringstream coordStream(tpLocCoords);
					std::string coordItem{};
					std::vector<float> coordValues{};
					while (std::getline(coordStream, coordItem, ',')) {
						try {
							coordValues.push_back(std::stof(coordItem));
						} catch (const std::invalid_argument& e) {
							UNREFERENCED_PARAMETER(e);
							spdlog::error("ParseTeleportLocations: Invalid coordinate value: {}, for location: {}", coordItem, tpLocName);
							break;
						}
					}

					Vector3 tpLocPos{};
					if (coordValues.size() == 3) {
						tpLocPos.X = coordValues[0];
						tpLocPos.Y = coordValues[1];
						tpLocPos.Z = coordValues[2];
						teleportLocations.push_back({ tpLocName, tpLocPos });
					} else
						spdlog::error("ParseTeleportLocations: Invalid number of coordinates ({}) for location: {}", coordValues.size(), tpLocName);
				} else
					spdlog::error("ParseTeleportLocations: Invalid format for TP location: {}", item);
			}

			spdlog::info("Successfully parsed teleport locations:");
			int tpLocIndex = 1;
			for (const auto& loc : teleportLocations) {
				spdlog::info("{}. \"{}\" (X: {}, Y: {}, Z: {})", tpLocIndex, loc.name, loc.pos.X, loc.pos.Y, loc.pos.Z);
				tpLocIndex++;
			}

			return teleportLocations;
		}
		std::string ConvertTeleportLocationsToStr(const std::vector<TeleportLocation>& teleportLocations) {
			std::stringstream ss{};

			for (size_t i = 0; i < teleportLocations.size(); ++i) {
				const TeleportLocation& loc = teleportLocations[i];

				ss << loc.name << ":" << loc.pos.X << "," << loc.pos.Y << "," << loc.pos.Z;
				if (i < teleportLocations.size() - 1)
					ss << ";";
			}

			return ss.str();
		}

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

				teleportCoords = camPos;
			} else {
				Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();
				if (!playerCharacter)
					return;

				teleportCoords = playerCharacter->playerPos;
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
				if (!teleportCoords.isDefault())
					teleportCoords = Vector3();
				return;
			}
			if (!teleportCoords.isDefault())
				return;

			if (Camera::freeCam.GetValue()) {
				GamePH::FreeCamera* freeCam = GamePH::FreeCamera::Get();
				if (!freeCam)
					return;

				Vector3 camPos{};
				freeCam->GetPosition(&camPos);
				if (camPos.isDefault())
					return;

				teleportCoords = camPos;
			} else {
				Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();
				if (!playerCharacter)
					return;

				teleportCoords = playerCharacter->playerPos;
			}
		}
		static void HotkeysUpdate() {
			teleportToSelectedLocation.SetChangesAreDisabled(selectedTPLocation < 0 || selectedTPLocation >= savedTeleportLocations.size());
			teleportToCoords.SetChangesAreDisabled(isTeleportationDisabled());

			if (teleportToSelectedLocation.HasChanged()) {
				TeleportPlayerTo(savedTeleportLocations[selectedTPLocation].pos);
				teleportToSelectedLocation.SetPrevValue(teleportToSelectedLocation.GetValue());
			}
			if (teleportToCoords.HasChanged()) {
				TeleportPlayerTo(teleportCoords);
				teleportToCoords.SetPrevValue(teleportToCoords.GetValue());
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
			playerPos = playerPos.round(1);

			if (savedLocIt != savedTeleportLocations.end() && savedLocIt->pos.round() == playerPos.round()) {
				ImGui::OpenPopup("Location already exists");
				return;
			}

			savedTeleportLocations.emplace_back(std::string(locationName), playerPos);
			savedTeleportLocationNames.clear();
			for (const auto& loc : savedTeleportLocations)
				savedTeleportLocationNames.emplace_back(loc.name.data());
		}
		static void HandleDialogs() {
			if (ImGui::BeginPopupModal("Give the location a name", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				if (ImGui::InputTextWithHint("##TPLocationNameInputText", "Location name", newLocationName, IM_ARRAYSIZE(newLocationName), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
					SaveTeleportLocation(newLocationName);
					newLocationName[0] = 0;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopupModal("Location already exists", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("The location you have entered already exists. Either the name of the location, or the position of the location is already inside the list. If you want to change it then please remove it and add it again.");
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
			HotkeysUpdate();
		}
		void Tab::Render() {
			ImGui::SeparatorText("Saved Locations##Teleport");
			ImGui::BeginDisabled(isTeleportationDisabled()); {
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::ListBox("##SavedTPLocationsListBox", &selectedTPLocation, savedTeleportLocationNames.data(), static_cast<int>(savedTeleportLocationNames.size()), 5);
				ImGui::PopItemWidth();

				ImGui::EndDisabled();
			}

			ImGui::BeginDisabled(isTeleportationDisabled() || selectedTPLocation < 0 || selectedTPLocation >= savedTeleportLocations.size()); {
				if (ImGui::ButtonHotkey("Teleport to Selected Location", &teleportToSelectedLocation, "Teleports player to selected location from the saved locations list"))
					TeleportPlayerTo(savedTeleportLocations[selectedTPLocation].pos);
				ImGui::SameLine();
				if (ImGui::Button("Remove Selected Location")) {
					savedTeleportLocations.erase(savedTeleportLocations.begin() + selectedTPLocation);
					savedTeleportLocationNames.clear();
					for (const auto& loc : savedTeleportLocations)
						savedTeleportLocationNames.emplace_back(loc.name.data());
					selectedTPLocation = -1;
				}
				ImGui::EndDisabled();
			}
			ImGui::BeginDisabled(isTeleportationDisabled()); {
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
					playerPos = "Player Position -- X: 0.00, Y: 0.00, Z: 0.00";
				else {
					playerPos = "Player Position -- X: " + std::format("{:.2f}", playerCharacter->playerPos.data.X) + ", Y: " + std::format("{:.2f}", playerCharacter->playerPos.data.Y) + ", Z: " + std::format("{:.2f}", playerCharacter->playerPos.data.Z);
				}

				static std::string cameraPos = "Free Camera Position -- X: 0.00, Y: 0.00, Z: 0.00";
				GamePH::FreeCamera* freeCam = GamePH::FreeCamera::Get();
				if (!Camera::freeCam.GetValue() || !freeCam)
					cameraPos = "Free Camera Position -- X: 0.00, Y: 0.00, Z: 0.00";
				else {
					Vector3 camPos{};
					freeCam->GetPosition(&camPos);
					if (camPos.isDefault())
						cameraPos = "Free Camera Position -- X: 0.00, Y: 0.00, Z: 0.00";
					else
						cameraPos = "Free Camera Position -- X: " + std::format("{:.2f}", camPos.X) + ", Y: " + std::format("{:.2f}", camPos.Y) + ", Z: " + std::format("{:.2f}", camPos.Z);
				}

				ImGui::Text(playerPos.data());
				ImGui::Text(cameraPos.data());

				ImGui::PushItemWidth(200.0f);
				ImGui::InputFloat("X", &teleportCoords.X, 1.0f, 10.0f, "%.2f");
				ImGui::SameLine();
				ImGui::InputFloat("Y", &teleportCoords.Y, 1.0f, 10.0f, "%.2f");
				ImGui::SameLine();
				ImGui::InputFloat("Z", &teleportCoords.Z, 1.0f, 10.0f, "%.2f");
				ImGui::PopItemWidth();

				if (ImGui::ButtonHotkey("Teleport to Coords", &teleportToCoords, "Teleports player to the coords specified in the input boxes above"))
					TeleportPlayerTo(teleportCoords);
				ImGui::SameLine();
				if (ImGui::Button("Get Player Coords"))
					SyncTPCoordsToPlayer();

				ImGui::EndDisabled();
			}

			HandleDialogs();
		}
	}
}