#include <pch.h>
#include "..\game\GamePH\GameDI_PH.h"
#include "..\game\GamePH\LevelDI.h"
#include "misc.h"

namespace Menu {
	namespace Misc {
		KeyBindOption disableHUD{ VK_F8 };
		KeyBindOption disableGamePauseWhileAFK{ VK_NONE };

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
			ImGui::BeginDisabled(disableHUD.GetChangesAreDisabled()); {
				ImGui::CheckboxHotkey("Disable HUD", &disableHUD);
				ImGui::EndDisabled();
			}
			ImGui::SameLine();
			ImGui::CheckboxHotkey("Disable Game Pause While AFK", &disableGamePauseWhileAFK);
		}
	}
}