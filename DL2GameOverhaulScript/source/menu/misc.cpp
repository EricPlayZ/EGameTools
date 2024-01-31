#include <Hotkey.h>
#include <ImGuiEx.h>
#include <imgui.h>
#include "..\game_classes.h"
#include "misc.h"

namespace Menu {
	namespace Misc {
		KeyBindOption disableHUD{ VK_F8 };

		Tab Tab::instance{};
		void Tab::Update() {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel)
				return;

			if (disableHUD.HasChanged()) {
				disableHUD.SetPrevValue(disableHUD.GetValue());
				iLevel->ShowUIManager(!disableHUD.GetValue());
			}
		}
		void Tab::Render() {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			ImGui::SeparatorText("Misc##Misc");
			ImGui::BeginDisabled(!iLevel->IsLoaded(), &disableHUD); {
				ImGui::Checkbox("Disable HUD", &disableHUD);
				ImGui::Hotkey("##GodModeToggleKey", disableHUD);
				ImGui::EndDisabled();
			}
			if (ImGui::Button("Reload Jump Params")) {
				char(*pReloadJumps)() = (decltype(pReloadJumps))reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(GetModuleHandle("gamedll_ph_x64_rwdi.dll")) + 0xF900A0);
				pReloadJumps();
			}
		}
	}
}