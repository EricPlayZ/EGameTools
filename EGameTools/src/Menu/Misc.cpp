#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex.h>
#include <EGSDK\Utils\Hook.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\GamePH\GameDI_PH.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\ImGui_impl\DeferredActions.h>
#include <EGT\Menu\Misc.h>
#include <EGT\GamePH\GamePH_Hooks.h>

namespace EGT::Menu {
	namespace Misc {
		ImGui::KeyBindOption disableGamePauseWhileAFK{ false, VK_NONE };
		ImGui::KeyBindOption disableHUD{ false, VK_F8 };
		ImGui::KeyBindOption disableTAA{ false, VK_NONE };
		ImGui::Option disableSavegameCRCCheck{ false };
		ImGui::Option disableDataPAKsCRCCheck{ false };
		ImGui::Option increaseDataPAKsLimit{ false };

		static char rendererCVarsSearchFilter[64];

		static void RendererCVarsUpdate() {
			EGSDK::Engine::CVars::ManageVarByBool("i_pp_taa_on", 0, 1, disableTAA.GetValue());
			EGSDK::Engine::CVars::ManageVarByBool("i_pp_jitter_on", 0, 1, disableTAA.GetValue());
		}

		static bool ShouldDisplayVariable(const std::unique_ptr<EGSDK::Engine::CVar>& cVarPtr, const std::string& searchFilter) {
			if (cVarPtr->GetType() == EGSDK::Engine::VarType::NONE)
				return false;
			if (searchFilter.empty())
				return true;

			// Convert searchFilter and variable name to lowercase
			std::string lowerFilter = EGSDK::Utils::Values::to_lower(searchFilter);
			std::string lowerKey = EGSDK::Utils::Values::to_lower(cVarPtr->GetName());
			return lowerKey.find(lowerFilter) != std::string::npos;
		}
		static void RestoreVariableToDefault(const std::unique_ptr<EGSDK::Engine::CVar>& cVarPtr) {
			auto cVar = EGSDK::Engine::CVars::GetVarRef(cVarPtr.get());

			ImGui_impl::DeferredActions::Add([cVar]() mutable {
				switch (cVar->GetType()) {
					case EGSDK::Engine::VarType::Float:
						cVar->RestoreVarToDefault<float>();
						break;
					case EGSDK::Engine::VarType::Int:
						cVar->RestoreVarToDefault<int>();
						break;
					case EGSDK::Engine::VarType::Vec3:
						cVar->RestoreVarToDefault<EGSDK::Vec3>();
						break;
					case EGSDK::Engine::VarType::Vec4:
						cVar->RestoreVarToDefault<EGSDK::Vec4>();
						break;
					default:
						break;
				}
			});
		}
		static void RestoreVariablesToDefault() {
			EGSDK::Engine::CVars::customVars.ForEach([](const std::unique_ptr<EGSDK::Engine::CVar>& cVarPtr) {
				RestoreVariableToDefault(cVarPtr);
			});
		}
		static void RenderRendererCVar(const std::unique_ptr<EGSDK::Engine::CVar>& cVarPtr) {
			auto cVar = EGSDK::Engine::CVars::GetVarRef(cVarPtr.get());

			ImGui::BeginDisabled(cVar->IsManagedByBool());
			switch (cVar->GetType()) {
				case EGSDK::Engine::VarType::Float:
				{
					auto value = cVar->GetValue<float>();
					if (!value)
						return;
					auto newValue = *value;
					if (ImGui::InputFloat(cVar->GetName(), &newValue))
						cVar->SetValueFromList(newValue);
					break;
				}
				case EGSDK::Engine::VarType::Int:
				{
					auto value = cVar->GetValue<int>();
					if (!value)
						return;
					auto newValue = *value;
					if (ImGui::InputInt(cVar->GetName(), &newValue))
						cVar->SetValueFromList(newValue);
					break;
				}
				case EGSDK::Engine::VarType::Vec3:
				{
					auto value = cVar->GetValue<EGSDK::Vec3>();
					if (!value)
						return;
					auto newValue = *value;
					if (ImGui::InputFloat3(cVar->GetName(), reinterpret_cast<float*>(&newValue)))
						cVar->SetValueFromList(newValue);
					break;
				}
				case EGSDK::Engine::VarType::Vec4:
				{
					auto value = cVar->GetValue<EGSDK::Vec4>();
					if (!value)
						return;
					auto newValue = *value;
					if (ImGui::InputFloat4(cVar->GetName(), reinterpret_cast<float*>(&newValue)))
						cVar->SetValueFromList(newValue);
					break;
				}
				default:
					break;
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			std::string restoreBtnName = "Restore##" + std::string(cVar->GetName());

			ImGui::BeginDisabled(!cVar->HasCustomValue() || cVar->IsManagedByBool());
			if (ImGui::Button(restoreBtnName.c_str(), "Restores renderer cvar to default"))
				RestoreVariableToDefault(cVarPtr);
			ImGui::EndDisabled();
		}
		static void HandleRendererCVarsList() {
			ImGui::BeginDisabled(!EGSDK::Engine::CVars::AreAnyVarsPresent());
			if (ImGui::CollapsingHeader("Renderer CVars list", ImGuiTreeNodeFlags_None)) {
				ImGui::Indent();

				ImGui::BeginDisabled(!EGSDK::Engine::CVars::AreAnyCustomVarsPresent() || EGSDK::Engine::CVars::AreAllCustomVarsManagedByBool());
				if (ImGui::Button("Restore variables to default"))
					RestoreVariablesToDefault();
				ImGui::EndDisabled();

				ImGui::Separator();
				ImGui::InputTextWithHint("##RendererCVarsSearch", "Search variables", rendererCVarsSearchFilter, 64);

				EGSDK::Engine::CVars::vars.ForEach([](std::unique_ptr<EGSDK::Engine::CVar>& cVarPtr) {
					if (ShouldDisplayVariable(cVarPtr, rendererCVarsSearchFilter))
						RenderRendererCVar(cVarPtr);
				});

				ImGui::Unindent();
			}
			ImGui::EndDisabled();
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
			ImGui::SeparatorText("Misc##Misc");
			ImGui::CheckboxHotkey("Disable Game Pause While AFK", &disableGamePauseWhileAFK, "Prevents the game from pausing while you're afk");
			ImGui::SameLine();
			ImGui::BeginDisabled(disableHUD.GetChangesAreDisabled());
			ImGui::CheckboxHotkey("Disable HUD", &disableHUD, "Disables the entire HUD, including any sort of menus like the pause menu");
			ImGui::EndDisabled();

			ImGui::CheckboxHotkey("Disable TAA", &disableTAA, "Disables the TAA/anti-aliasing (only works if you have upscaling disabled)");

			ImGui::SeparatorText("Scripting##Misc");
			HandleRendererCVarsList();

			ImGui::SeparatorText("Game Checks##Misc");
			if (ImGui::Checkbox("Disable Savegame CRC Check *", &disableSavegameCRCCheck, "Stops the game from falsely saying your savegame is corrupt whenever you modify it outside of the game using a save editor"))
				disableSavegameCRCCheck.GetValue() ? EGT::GamePH::Hooks::SaveGameCRCBoolCheckHook.Enable() : EGT::GamePH::Hooks::SaveGameCRCBoolCheckHook.Disable();
			ImGui::SameLine();
			ImGui::Checkbox("Disable Data PAKs CRC Check *", &disableDataPAKsCRCCheck, "Stops the game from scanning data PAKs, which allows you to use data PAK mods in multiplayer as well");
			ImGui::Checkbox("Increase Data PAKs Limit *", &increaseDataPAKsLimit, "Allows you to add more than 8 data PAKs, e.g. data8.pak, data9.pak, data10.pak, etc, up to 200 PAKs in total");
			ImGui::Separator();
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "* Option requires game restart to apply");
		}
	}
}