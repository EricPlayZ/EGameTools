#include <algorithm>
#include "..\ImGui\imgui.h"
#include "..\game_classes.h"
#include "..\core.h"
#include "camera.h"

namespace Menu {
	namespace Player {
		SMART_BOOL godModeEnabled{};
		SMART_BOOL freezePlayerEnabled{};

		static char searchFilter[64];

		void PlayerPositionUpdate() {
			Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();
			if (!playerCharacter)
				return;

			if (!Menu::Player::freezePlayerEnabled.value) {
				Engine::CBulletPhysicsCharacter::posBeforeFreeze = playerCharacter->playerPos;

				if (!Menu::Camera::freeCamEnabled)
					return;
				if (!Menu::Camera::teleportPlayerToCameraEnabled)
					return;

				GamePH::FreeCamera* freeCam = GamePH::FreeCamera::Get();
				if (!freeCam)
					return;

				Vector3 camPos{};
				freeCam->GetPosition(&camPos);

				if (camPos.isDefault())
					return;

				playerCharacter->MoveCharacter(camPos);
			} else
				playerCharacter->FreezeCharacter();
		}

		void Update() {
			if (Menu::Camera::freeCamEnabled)
				godModeEnabled.Change(true);
			else if (!Menu::Camera::freeCamEnabled)
				godModeEnabled.Restore();

			if (Menu::Camera::freeCamEnabled)
				freezePlayerEnabled.Change(!Menu::Camera::teleportPlayerToCameraEnabled);
			else if (!Menu::Camera::freeCamEnabled)
				freezePlayerEnabled.Restore();

			PlayerPositionUpdate();
		}

		void Render() {
			ImGui::BeginDisabled(!GamePH::PlayerHealthModule::Get() || Menu::Camera::freeCamEnabled); {
				ImGui::Checkbox("God Mode", &godModeEnabled.value);
				ImGui::EndDisabled();
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(!Engine::CBulletPhysicsCharacter::Get() || Menu::Camera::freeCamEnabled); {
				ImGui::Checkbox("Freeze Player", &freezePlayerEnabled.value);
				ImGui::EndDisabled();
			}
			ImGui::BeginDisabled(!GamePH::PlayerVariables::gotPlayerVars); {
				if (ImGui::CollapsingHeader("Player Variables", ImGuiTreeNodeFlags_None)) {
					ImGui::InputText("##VarsSearch", searchFilter, 32);

					for (auto const& [key, val] : GamePH::PlayerVariables::unorderedPlayerVars) {
						if (!val.first)
							continue;

						std::string lowerSearch = key.data();
						std::string lowerFilter = searchFilter;
						std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), tolower);
						std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), tolower);
						if (lowerSearch.find(std::string(lowerFilter)) == std::string::npos)
							continue;

						if (val.second == "float") {
							float* varAddr = reinterpret_cast<float*>(val.first);
							ImGui::InputFloat(key.data(), &*varAddr);
							*(varAddr + 1) = *varAddr;
						} else if (val.second == "bool") {
							bool* varAddr = reinterpret_cast<bool*>(val.first);
							ImGui::Checkbox(key.data(), &*varAddr);
							*(varAddr + 1) = *varAddr;
						}
					}
				}
				ImGui::EndDisabled();
			}
		}
	}
}