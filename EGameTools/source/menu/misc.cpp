#include <pch.h>
#include "..\game\GamePH\GameDI_PH.h"
#include "..\game\GamePH\LevelDI.h"
#include "misc.h"

namespace GamePH::Hooks {
	extern Utils::Hook::ByteHook<LPVOID> SaveGameCRCBoolCheckHook;
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
			ImGui::CheckboxHotkey("Disable Game Pause While AFK", &disableGamePauseWhileAFK, "Prevents the game from pausing while you're afk");
			ImGui::SameLine();
			ImGui::BeginDisabled(disableHUD.GetChangesAreDisabled()); {
				ImGui::CheckboxHotkey("Disable HUD", &disableHUD, "Disables the entire HUD, including any sort of menus like the pause menu");
				ImGui::EndDisabled();
			}
			ImGui::SeparatorText("Game Checks##Misc");
			if (ImGui::Checkbox("Disable Savegame CRC Check *", &disableSavegameCRCCheck, "Stops the game from falsely saying your savegame is corrupt whenever you modify it outside of the game using a save editor"))
				GamePH::Hooks::SaveGameCRCBoolCheckHook.Toggle();
			ImGui::SameLine();
			ImGui::Checkbox("Disable Data PAKs CRC Check *", &disableDataPAKsCRCCheck, "Stops the game from scanning data PAKs, which allows you to use data PAK mods in multiplayer as well");
			ImGui::Checkbox("Increase Data PAKs Limit *", &increaseDataPAKsLimit, "Allows you to add more than 8 data PAKs, e.g. data8.pak, data9.pak, data10.pak, etc, up to 200 PAKs in total");
			ImGui::Separator();
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "* Option requires game restart to apply");
		}
	}
}