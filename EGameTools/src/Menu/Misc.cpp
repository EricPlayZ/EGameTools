#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex.h>
#include <EGSDK\Utils\Hook.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\GamePH\GameDI_PH.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\ImGui_impl\DeferredActions.h>
#include <EGT\GamePH\GamePH_Hooks.h>
#include <EGT\Menu\Misc.h>
#include <EGT\Menu\VarList.h>

namespace EGT::Menu {
	namespace Misc {
		ImGui::KeyBindOption disableGamePauseWhileAFK{ false, VK_NONE };
		ImGui::KeyBindOption disableHUD{ false, VK_F8 };
		ImGui::KeyBindOption disableTAA{ false, VK_NONE };
		ImGui::Option disableSavegameCRCCheck{ false };
		ImGui::Option disableDataPAKsCRCCheck{ false };
		ImGui::Option increaseDataPAKsLimit{ false };

		static VarList<EGSDK::Engine::CVars> cVarsList{ "Renderer CVars List" };

		static void RendererCVarsUpdate() {
			EGSDK::Engine::CVars::ManageVarByBool("i_pp_taa_on", 0, 1, disableTAA.GetValue());
			EGSDK::Engine::CVars::ManageVarByBool("i_pp_jitter_on", 0, 1, disableTAA.GetValue());
		}

		static void UpdateDisabledOptions() {
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			disableHUD.SetChangesAreDisabled(!iLevel || !iLevel->IsLoaded());
		}

		Tab Tab::instance{};
		void Tab::Init() {}
		void Tab::Update() {
			UpdateDisabledOptions();

			RendererCVarsUpdate();

			auto iLevel = EGSDK::GamePH::LevelDI::Get();
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

			auto gameDI_PH = EGSDK::GamePH::GameDI_PH::Get();
			if (gameDI_PH)
				gameDI_PH->blockPauseGameOnPlayerAfk = disableGamePauseWhileAFK.GetValue();
		}
		void Tab::Render() {
			ImGui::SeparatorTextSection("Misc##Misc", false);
			ImGui::CheckboxHotkey("Disable Game Pause While AFK", &disableGamePauseWhileAFK, "Prevents the game from pausing while you're afk");
			ImGui::BeginDisabled(disableHUD.GetChangesAreDisabled());
			ImGui::CheckboxHotkey("Disable HUD", &disableHUD, "Disables the entire HUD, including any sort of menus like the pause menu");
			ImGui::EndDisabled();

			ImGui::CheckboxHotkey("Disable TAA", &disableTAA, "Disables the TAA/anti-aliasing (only works if you have upscaling disabled)");

			ImGui::SeparatorTextSection("Scripting##Misc");
			cVarsList.Render();

			ImGui::SeparatorTextSection("Game Checks##Misc");
			if (ImGui::Checkbox("Disable Savegame CRC Check *", &disableSavegameCRCCheck, "Stops the game from falsely saying your savegame is corrupt whenever you modify it outside of the game using a save editor"))
				disableSavegameCRCCheck.GetValue() ? EGT::GamePH::Hooks::SaveGameCRCBoolCheckHook.Enable() : EGT::GamePH::Hooks::SaveGameCRCBoolCheckHook.Disable();
			ImGui::Checkbox("Disable Data PAKs CRC Check *", &disableDataPAKsCRCCheck, "Stops the game from scanning data PAKs, which allows you to use data PAK mods in multiplayer as well");
			ImGui::Checkbox("Increase Data PAKs Limit *", &increaseDataPAKsLimit, "Allows you to add more than 8 data PAKs, e.g. data8.pak, data9.pak, data10.pak, etc, up to 200 PAKs in total");
			ImGui::Separator();
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "* Option requires game restart to apply");
		}
	}
}