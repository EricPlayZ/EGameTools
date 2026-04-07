#include <cmath>
#include <spdlog\spdlog.h>
#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex.h>
#include <EGSDK\vec2.h>
#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\Menu\Camera.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\Player.h>
#include <EGT\Menu\Teleport.h>

namespace EGT::Menu {
	namespace Teleport {
		std::string savedTeleportLocationsStr = "Bazaar - Highest Point:1944.4,123.6,931.9,-111.1,-16.3;Bazaar - Main Entrance:1962.8,50.1,928,-148.1,-0.9;Colonel Williams Stronghold - Main Entrance Bridge:994.3,22.8,-1138.6,-225,-3.3;Dynamo Cars Factory - Main Entrance:2295.9,-2.1,-78.6,11.4,-0.3;Fish Eye - Player Safehouse:1180.4,32.4,-146.8,90.4,-14.5;Fish Eye - Top of The Baloon:1122.6,98.8,-101.2,-35.5,-15.9;Observatory - Meeting Room:1951.2,-13.4,-329.6,89.5,-2.9;Observatory - The 2 Domes (No Chemicals):1985.4,19.9,-357.2,366.8,20.9;Out of Bounds - Cut Road Quest:2693.3,-4.7,-241.5,325.3,-3.4;PK Metro Station - Main Entrance:1886.9,50,628.9,661.7,-1.7;PK Ship - Main Entrance:801.8,4.2,139.8,781.1,-2.7;St. Paul Cathedral - GRE Entrance:463.4,4.2,-421,900,-1.1;Tolga & Fatin Quest - Underground Loot Box:2343.9,12.2,-661.5,1259.7,-0.8;VNC Tower - \"V\" Logo:1434.2,4.3,-319.3,1350.7,14.6;VNC Tower - Highest Player Safehouse:1424.8,354.6,-457.9,1171.2,-3.8;VNC Tower - Highest Point:1403.8,446.7,-389.8,1887.3,-19;X13 - Tunnel Near the City Walls Towards Facility:2407.9,36.2,-461.7,1919.8,-0.9;X13 - Underground Facility:2437.8,12.2,-649.9,1439.6,-2;X13 - Waltz Arena:2551.9,15.3,-569.1,1348.5,1.6;E3 2019 Truck Chase - Tall Building:1018.2,43.6,-213.9,-154.5,-7.4";
		std::vector<TeleportLocation> savedTeleportLocations{};
		static std::vector<std::string> savedTeleportLocationNames{};
		static std::vector<const char*> savedTeleportLocationNamesPtrs{};
		static int selectedTPLocation = -1;
		static char newLocationName[125]{};

		vec3 waypointCoords{};
		bool* waypointIsSet = nullptr;
		bool justTeleportedToWaypoint = false;
		static vec3 teleportCoords{};

		ImGui::KeyBindOption teleportToSelectedLocation{ false, VK_F9 };
		ImGui::KeyBindOption teleportToCoords{ false, VK_NONE };
		ImGui::KeyBindOption teleportToWaypoint{ false, VK_F10 };

		void UpdateTeleportLocationVisualNames() {
			savedTeleportLocationNames.clear();
			savedTeleportLocationNamesPtrs.clear();
			for (const auto& loc : savedTeleportLocations) {
				std::string completeName = loc.name + " (X: " + std::format("{:.1f}", loc.pos.X) + ", Y: " + std::format("{:.1f}", loc.pos.Y) + ", Z: " + std::format("{:.1f}", loc.pos.Z) + ")";
				savedTeleportLocationNames.emplace_back(completeName);
			}
			for (const auto& name : savedTeleportLocationNames)
				savedTeleportLocationNamesPtrs.emplace_back(name.c_str());
		}
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
					std::string coordStreamLine{};
					std::vector<float> coordValues{};
					while (std::getline(coordStream, coordStreamLine, ',')) {
						try {
							coordValues.push_back(std::stof(coordStreamLine));
						} catch (...) {
							SPDLOG_ERROR("Invalid coordinate value: {}, for location: {}", coordStreamLine, tpLocName);
							break;
						}
					}

					if (coordValues.size() == 3)
						teleportLocations.push_back({ tpLocName, { coordValues[0], coordValues[1], coordValues[2] } });
					else if (coordValues.size() == 5)
						teleportLocations.push_back({ tpLocName, { coordValues[0], coordValues[1], coordValues[2] }, { coordValues[3], coordValues[4] } });
					else
						SPDLOG_ERROR("Invalid number of coordinates ({}) for location: {}", coordValues.size(), tpLocName);
				} else
					SPDLOG_ERROR("Invalid format for TP location: {}", item);
			}

			SPDLOG_INFO("Successfully parsed teleport locations:");
			int tpLocIndex = 1;
			for (const auto& loc : teleportLocations) {
				SPDLOG_INFO("{}. \"{}\" (X: {}, Y: {}, Z: {}, Yaw: {}, Pitch: {})", tpLocIndex, loc.name, loc.pos.X, loc.pos.Y, loc.pos.Z, loc.orientation.X, loc.orientation.Y);
				tpLocIndex++;
			}

			return teleportLocations;
		}
		std::string ConvertTeleportLocationsToStr(const std::vector<TeleportLocation>& teleportLocations) {
			std::stringstream ss{};

			for (size_t i = 0; i < teleportLocations.size(); ++i) {
				const TeleportLocation& loc = teleportLocations[i];
				ss << loc.name << ":" << loc.pos.X << "," << loc.pos.Y << "," << loc.pos.Z << "," << loc.orientation.X << "," << loc.orientation.Y;
				if (i < teleportLocations.size() - 1)
					ss << ";";
			}

			return ss.str();
		}
		static std::string GetFormattedPosition(const vec3* position) {
			if (!position || position->isDefault())
				return "X: 0.00, Y: 0.00, Z: 0.00";
			static std::string formattedStr{};
			formattedStr = std::format("X: {:.2f}, Y: {:.2f}, Z: {:.2f}", position->X, position->Y, position->Z);
			return formattedStr;
		}

		static bool isTeleportationDisabled() {
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return true;
			if (!Camera::freeCam.GetValue() && !EGSDK::Engine::CBulletPhysicsCharacter::Get())
				return true;
			else if (Camera::freeCam.GetValue() && !EGSDK::GamePH::FreeCamera::Get())
				return true;

			return false;
		}

		static void SyncPlayerCoordsToTPCoords() {
			if (isTeleportationDisabled())
				return;

			if (Camera::freeCam.GetValue()) {
				auto freeCam = EGSDK::GamePH::FreeCamera::Get();
				if (freeCam)
					freeCam->GetPosition(&teleportCoords);
			} else {
				auto playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
				if (playerCharacter)
					teleportCoords = playerCharacter->playerPos;
			}
		}
		static bool TeleportPlayerTo(const vec3& pos, const vec2& orientation = {}) {
			if (isTeleportationDisabled() || pos.isDefault()) {
				if (pos.isDefault())
					SPDLOG_ERROR("Teleport position was default, couldn't teleport player");
				return false;
			}

			if (Camera::freeCam.GetValue()) {
				auto freeCam = EGSDK::GamePH::FreeCamera::Get();
				if (!freeCam)
					return false;
				freeCam->SetPosition(&pos);
			} else {
				auto playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
				if (!playerCharacter)
					return false;
				auto playerDI_PH = EGSDK::GamePH::PlayerDI_PH::Get();
				if (!playerDI_PH && !orientation.isDefault())
					SPDLOG_ERROR("PlayerDI_PH was null, won't be able to set player teleport orientation");

				if (Player::freezePlayer.GetValue())
					playerCharacter->posBeforeFreeze = pos;

				playerCharacter->MoveCharacter(pos);
				if (playerDI_PH && !orientation.isDefault())
					playerDI_PH->nextPlayerOrientation->X = orientation.X;
			}
			
			return true;
		}

		static void UpdateTeleportPos() {
			if (isTeleportationDisabled()) {
				if (!teleportCoords.isDefault())
					teleportCoords = {};
				return;
			}
			if (!teleportCoords.isDefault())
				return;

			if (Camera::freeCam.GetValue()) {
				auto freeCam = EGSDK::GamePH::FreeCamera::Get();
				if (freeCam)
					freeCam->GetPosition(&teleportCoords);
			} else {
				auto playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
				if (playerCharacter)
					teleportCoords = playerCharacter->playerPos;
			}
		}
		static void HotkeysUpdate() {
			teleportToSelectedLocation.SetChangesAreDisabled(selectedTPLocation < 0 || selectedTPLocation >= savedTeleportLocations.size());
			teleportToWaypoint.SetChangesAreDisabled(isTeleportationDisabled() || !waypointIsSet || !*waypointIsSet);
			teleportToCoords.SetChangesAreDisabled(isTeleportationDisabled());

			if (teleportToSelectedLocation.HasChanged()) {
				TeleportPlayerTo(savedTeleportLocations[selectedTPLocation].pos, savedTeleportLocations[selectedTPLocation].orientation);
				teleportToSelectedLocation.SetPrevValue(teleportToSelectedLocation.GetValue());
			}
			if (teleportToWaypoint.HasChanged()) {
				justTeleportedToWaypoint = TeleportPlayerTo(waypointCoords);
				teleportToWaypoint.SetPrevValue(teleportToWaypoint.GetValue());
			}
			if (teleportToCoords.HasChanged()) {
				TeleportPlayerTo(teleportCoords);
				teleportToCoords.SetPrevValue(teleportToCoords.GetValue());
			}
		}

		static bool SaveTeleportLocation(const char* locationName, bool overwrite = false) {
			if (isTeleportationDisabled()) {
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				ImGui::OpenPopup("Couldn't add location");
				return false;
			}

			if (!overwrite) {
				auto savedLocIt = std::find_if(savedTeleportLocations.begin(), savedTeleportLocations.end(), [&locationName](const auto& loc) {
					return loc.name == locationName;
				});
				if (savedLocIt != savedTeleportLocations.end()) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::OpenPopup("Location with same name already exists");
					return false;
				}
			}

			vec3 playerPos{};
			vec2 playerOrientation{};

			if (Camera::freeCam.GetValue()) {
				EGSDK::GamePH::FreeCamera* freeCam = EGSDK::GamePH::FreeCamera::Get();
				if (!freeCam) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::OpenPopup("Couldn't add location");
					SPDLOG_ERROR("Couldn't add location because FreeCamera was null");
					return false;
				}

				vec3 camPos{};
				freeCam->GetPosition(&camPos);
				vec3 camForwardVec{};
				freeCam->GetForwardVector(&camForwardVec);
				if (camPos.isDefault() || camForwardVec.isDefault()) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::OpenPopup("Couldn't add location");
					SPDLOG_ERROR("Couldn't add location because {} was null", camPos.isDefault() ? "camPos" : "camForwardVec");
					return false;
				}

				playerPos = camPos;
				playerOrientation = { EGSDK::Utils::Values::GetPitchDegreesRelativeTo(camForwardVec, { 1.0f, 0.0f, 0.0f }), EGSDK::Utils::Values::GetPitchDegreesRelativeTo(camForwardVec, { 0.0f, 1.0f, 0.0f }) };
			} else {
				EGSDK::Engine::CBulletPhysicsCharacter* playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
				EGSDK::GamePH::PlayerDI_PH* playerDI_PH = EGSDK::GamePH::PlayerDI_PH::Get();
				if (!playerCharacter || !playerDI_PH) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::OpenPopup("Couldn't add location");
					SPDLOG_ERROR("Couldn't add location because {} was null", !playerCharacter ? "CBulletPhysicsCharacter" : "PlayerDI_PH");
					return false;
				}

				playerPos = playerCharacter->playerPos;
				playerOrientation = playerDI_PH->nextPlayerOrientation;
			}
			playerPos = playerPos.round(1);
			playerOrientation = playerOrientation.round(1);

			if (!overwrite) {
				auto savedLocIt = std::find_if(savedTeleportLocations.begin(), savedTeleportLocations.end(), [&playerPos](const auto& loc) {
					return loc.pos == playerPos;
				});
				if (savedLocIt != savedTeleportLocations.end() && savedLocIt->pos == playerPos) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::OpenPopup("Location with same position already exists");
					return false;
				}

				savedTeleportLocations.emplace_back(std::string(locationName), playerPos, playerOrientation);
				UpdateTeleportLocationVisualNames();
			} else {
				auto savedLocIt = std::find_if(savedTeleportLocations.begin(), savedTeleportLocations.end(), [&locationName](const auto& loc) {
					return loc.name == locationName;
				});
				if (savedLocIt == savedTeleportLocations.end()) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					ImGui::OpenPopup("Couldn't add location");
					SPDLOG_ERROR("Couldn't overwrite location {} because it wasn't found in the list", locationName);
					return false;
				}

				savedLocIt->name = locationName;
				savedLocIt->pos = playerPos;
				savedLocIt->orientation = playerOrientation;

				UpdateTeleportLocationVisualNames();
			}

			ImGui::CloseCurrentPopup();
			return true;
		}
		static void HandleDialogs() {
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Give the location a name", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				bool tpSaveResult = true;

				ImGui::SetNextItemWidth(510.0f * Menu::scale);
				if (ImGui::InputTextWithHint("##TPLocationNameInputText", "Location name", newLocationName, IM_ARRAYSIZE(newLocationName), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("OK", ImVec2(250.0f, 0.0f) * Menu::scale) && newLocationName[0]) {
					tpSaveResult = SaveTeleportLocation(newLocationName);
					if (tpSaveResult)
						newLocationName[0] = 0;
				}
				else if (ImGui::SameLine(); ImGui::Button("Cancel", ImVec2(250.0f, 0.0f) * Menu::scale))
					ImGui::CloseCurrentPopup();
				if (tpSaveResult)
					ImGui::EndPopup();
			}
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Location with same name already exists", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				bool tpSaveResult = true;

				ImGui::SetNextItemWidth(510.0f * Menu::scale);
				ImGui::TextCentered("The location with name \"%s\" already exists. Would you like to overwrite that location with the new one?", newLocationName);

				if (ImGui::Button("OK", ImVec2(250.0f, 0.0f) * Menu::scale)) {
					tpSaveResult = SaveTeleportLocation(newLocationName, true);
					newLocationName[0] = 0;
				} else if (ImGui::SameLine(); ImGui::Button("Cancel", ImVec2(250.0f, 0.0f) * Menu::scale)) {
					newLocationName[0] = 0;
					ImGui::CloseCurrentPopup();
				}
				if (tpSaveResult)
					ImGui::EndPopup();
			}
			ImGui::DisplaySimplePopupMessageCentered(500.0f, Menu::scale, "Location with same position already exists", "The location with the same position you have entered already exists. If you want to change it then please remove it and add it again.");
			ImGui::DisplaySimplePopupMessageCentered(500.0f, Menu::scale, "Couldn't add location", "Something went wrong trying to add a location. Either the player class or camera class are not found, or you're in a place in the game where the character or camera isn't properly loaded. If this happens, even though you know it should work fine, please contact @EricPlayZ on NexusMods, GitHub or Discord.");
		}

		Tab Tab::instance{};
		void Tab::Init() {}
		void Tab::Update() {
			UpdateTeleportPos();
			HotkeysUpdate();
		}
		void Tab::Render() {
			ImGui::SeparatorTextSection("Saved Locations##Teleport", false);
			{
				const ImGuiStyle& tpSt = ImGui::GetStyle();
				const float lh = ImGui::GetTextLineHeightWithSpacing();
				const float listH = lh * 5.5f + tpSt.FramePadding.y * 2.0f + 6.0f;
				const float listW = std::fmax(80.0f, ImGui::GetContentRegionAvail().x - 8.0f);
				ImGui::PushItemWidth(listW);
				if (ImGui::BeginListBox("##SavedTPLocationsListBox", ImVec2(0.0f, listH))) {
					ImGuiListClipper clipper;
					const int n = static_cast<int>(savedTeleportLocationNamesPtrs.size());
					clipper.Begin(n, lh);
					while (clipper.Step()) {
						for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
							ImGui::PushID(i);
							const bool sel = (i == selectedTPLocation);
							if (ImGui::Selectable(savedTeleportLocationNamesPtrs[static_cast<size_t>(i)], sel))
								selectedTPLocation = i;
							if (sel)
								ImGui::SetItemDefaultFocus();
							ImGui::PopID();
						}
					}
					ImGui::EndListBox();
				}
				ImGui::PopItemWidth();
			}

			{
				const float btnW = ImGui::GetContentRegionAvail().x;
				ImGui::BeginDisabled(isTeleportationDisabled() || selectedTPLocation < 0 || selectedTPLocation >= savedTeleportLocations.size());
				if (ImGui::ButtonHotkey("Teleport to Selected Location", &teleportToSelectedLocation, "Teleports player to selected location from the saved locations list", ImVec2(btnW, 0.0f)))
					TeleportPlayerTo(savedTeleportLocations[selectedTPLocation].pos, savedTeleportLocations[selectedTPLocation].orientation);
				ImGui::EndDisabled();
			}
			{
				const float btnW = ImGui::GetContentRegionAvail().x;
				ImGui::BeginDisabled(isTeleportationDisabled());
				if (ImGui::Button("Save Current Location", nullptr, ImVec2(btnW, 0.0f)))
					ImGui::OpenPopup("Give the location a name");
				ImGui::EndDisabled();
			}
			{
				const float btnW = ImGui::GetContentRegionAvail().x;
				ImGui::BeginDisabled(selectedTPLocation < 0 || selectedTPLocation >= savedTeleportLocations.size() || isTeleportationDisabled());
				if (ImGui::Button("Overwrite Selected Location", nullptr, ImVec2(btnW, 0.0f)))
					SaveTeleportLocation(savedTeleportLocations[selectedTPLocation].name.c_str(), true);
				ImGui::EndDisabled();
			}
			{
				const float btnW = ImGui::GetContentRegionAvail().x;
				ImGui::BeginDisabled(selectedTPLocation < 0 || selectedTPLocation >= savedTeleportLocations.size());
				if (ImGui::Button("Remove Selected Location", nullptr, ImVec2(btnW, 0.0f))) {
					savedTeleportLocations.erase(savedTeleportLocations.begin() + selectedTPLocation);
					UpdateTeleportLocationVisualNames();
					selectedTPLocation = -1;
				}
				ImGui::EndDisabled();
			}

			ImGui::SeparatorTextSection("Custom##Teleport");
			ImGui::BeginDisabled(isTeleportationDisabled());
			auto playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
			auto freeCam = EGSDK::GamePH::FreeCamera::Get();

			vec3 camPos{};
			ImGui::Text("Player Position: %s", GetFormattedPosition(playerCharacter ? &*playerCharacter->playerPos.getPointer() : nullptr).data());
			ImGui::Text("Free Camera Position: %s", GetFormattedPosition(freeCam && Camera::freeCam.GetValue() ? freeCam->GetPosition(&camPos) : nullptr).data());
			ImGui::Text("Waypoint Position: %s", GetFormattedPosition(waypointIsSet && *waypointIsSet && !waypointCoords.isDefault() ? &waypointCoords : nullptr).data());

			ImGui::InputFloat3Stacked("Teleport Coords (XYZ)", reinterpret_cast<float*>(&teleportCoords), "%.2fm");
			ImGui::EndDisabled();

			{
				const float btnW = ImGui::GetContentRegionAvail().x;
				ImGui::BeginDisabled(isTeleportationDisabled() || !waypointIsSet || !*waypointIsSet);
				if (ImGui::ButtonHotkey("Teleport to Waypoint", &teleportToWaypoint, "Teleports player to waypoint.\nWARNING: If the waypoint is selected to track an object/item on the map, Teleport to Waypoint will not work, if so just set the waypoint nearby instead.\nWARNING: Your player height won't change when teleporting, so make sure you catch yourself if you fall under the map because of the teleportation", ImVec2(btnW, 0.0f)))
					justTeleportedToWaypoint = TeleportPlayerTo(waypointCoords);
				ImGui::EndDisabled();
			}
			ImGui::BeginDisabled(isTeleportationDisabled());
			{
				const float btnW = ImGui::GetContentRegionAvail().x;
				if (ImGui::ButtonHotkey("Teleport to Coords", &teleportToCoords, "Teleports player to the coords specified in the input boxes above", ImVec2(btnW, 0.0f)))
					TeleportPlayerTo(teleportCoords);
			}
			{
				const float btnW = ImGui::GetContentRegionAvail().x;
				if (ImGui::Button("Get Player Coords", nullptr, ImVec2(btnW, 0.0f)))
					SyncPlayerCoordsToTPCoords();
			}
			ImGui::EndDisabled();

			HandleDialogs();
		}
	}
}