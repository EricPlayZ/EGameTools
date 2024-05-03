#include <pch.h>
#include "..\game\GamePH\GameDI_PH.h"
#include "..\game\GamePH\LevelDI.h"
#include "misc.h"

namespace GamePH::Hooks {
	extern Utils::Hook::BytesHook<LPVOID> SaveGameCRCBoolCheckHook;
}

namespace Menu {
	namespace Misc {
		KeyBindOption disableGamePauseWhileAFK{ VK_NONE };
		KeyBindOption disableHUD{ VK_F8 };
		Option disableSavegameCRCCheck{};
		Option disableDataPAKsCRCCheck{};
		Option increaseDataPAKsLimit{};

		static void UpdateDisabledOptions() {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			disableHUD.SetChangesAreDisabled(!iLevel || !iLevel->IsLoaded());
		}

		Tab Tab::instance{};
		void Tab::Update() {
			UpdateDisabledOptions();

			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel)
				return;

			if (!iLevel->IsLoaded() && disableHUD.GetValue()) {
				disableHUD.SetBothValues(false);
				iLevel->ShowUIManager(true);

				return;
			}
			if (disableHUD.HasChanged()) {
				disableHUD.SetPrevValue(disableHUD.GetValue());
				iLevel->ShowUIManager(!disableHUD.GetValue());
			}

			GamePH::GameDI_PH* gameDI_PH = GamePH::GameDI_PH::Get();
			if (!gameDI_PH)
				return;
			gameDI_PH->blockPauseGameOnPlayerAfk = disableGamePauseWhileAFK.GetValue();
		}
		void Tab::Render() {
			ImGui::SeparatorText("Misc##Misc");
			ImGui::CheckboxHotkey("Disable Game Pause While AFK", &disableGamePauseWhileAFK);
			ImGui::BeginDisabled(disableHUD.GetChangesAreDisabled()); {
				ImGui::CheckboxHotkey("Disable HUD", &disableHUD);
				ImGui::EndDisabled();
			}
			if (ImGui::Checkbox("Disable Savegame CRC Check *", &disableSavegameCRCCheck))
				GamePH::Hooks::SaveGameCRCBoolCheckHook.Toggle();
			ImGui::Checkbox("Disable Data PAKs CRC Check *", &disableDataPAKsCRCCheck);
			ImGui::Checkbox("Increase Data PAKs Limit *", &increaseDataPAKsLimit);
			ImGui::Separator();
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "* Option requires game restart to apply");
		}
	}
}