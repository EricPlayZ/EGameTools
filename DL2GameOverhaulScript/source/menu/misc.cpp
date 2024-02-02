#include <Hotkey.h>
#include <ImGuiEx.h>
#include <imgui.h>
#include "..\game_classes.h"
#include "misc.h"

namespace Menu {
	namespace Misc {
		KeyBindOption disableHUD{ VK_F8 };

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
		}
		void Tab::Render() {
			ImGui::SeparatorText("Misc##Misc");
			ImGui::BeginDisabled(disableHUD.GetChangesAreDisabled()); {
				ImGui::Checkbox("Disable HUD", &disableHUD);
				ImGui::Hotkey("##DisableHUDToggleKey", disableHUD);
				ImGui::EndDisabled();
			}
		}
	}
}