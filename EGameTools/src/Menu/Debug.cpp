#include <ImGui\imguiex.h>
#include <EGSDK\ClassHelpers.h>
#include <EGT\ImGui_impl\Win32_impl.h>
#include <EGT\Menu\Debug.h>

#include <EGSDK\GamePH\CoPlayerRestrictions.h>
#include <EGSDK\GamePH\DayNightCycle.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\GamePH\GameDI_PH.h>
#include <EGSDK\GamePH\GameDI_PH2.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\LocalClientDI.h>
#include <EGSDK\GamePH\LogicalPlayer.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\GamePH\PlayerHealthModule.h>
#include <EGSDK\GamePH\PlayerInfectionModule.h>
#include <EGSDK\GamePH\PlayerState.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGSDK\GamePH\SessionCooperativeDI.h>
#include <EGSDK\GamePH\TPPCameraDI.h>
#include <EGSDK\GamePH\TimeWeather\CSystem.h>

#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\Engine\CGSObject.h>
#include <EGSDK\Engine\CGSObject2.h>
#include <EGSDK\Engine\CGame.h>
#include <EGSDK\Engine\CInput.h>
#include <EGSDK\Engine\CLevel.h>
#include <EGSDK\Engine\CLevel2.h>
#include <EGSDK\Engine\CLobbySteam.h>
#include <EGSDK\Engine\CVideoSettings.h>
#include <EGSDK\Engine\CoPhysicsProperty.h>
#include <EGSDK\Engine\RTTIManager.h>
#include <EGSDK\Engine\CRTTI.h>

namespace EGT::Menu {
	namespace Debug {
		static const std::vector<std::pair<std::string_view, void*(*)()>> GamePHClassAddrMap = {
			{ "TimeWeather\\CSystem", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::TimeWeather::CSystem::Get)},
			{ "CoPlayerRestrictions", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::CoPlayerRestrictions::Get) },
			{ "DayNightCycle", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::DayNightCycle::Get) },
			{ "FreeCamera", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::FreeCamera::Get) },
			{ "GameDI_PH", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::GameDI_PH::Get) },
			{ "GameDI_PH2", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::GameDI_PH2::Get) },
			{ "LevelDI", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::LevelDI::Get) },
			{ "LocalClientDI", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::LocalClientDI::Get) },
			{ "LogicalPlayer", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::LogicalPlayer::Get) },
			{ "PlayerDI_PH", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::PlayerDI_PH::Get) },
			{ "PlayerHealthModule", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::PlayerHealthModule::Get) },
			{ "PlayerInfectionModule", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::PlayerInfectionModule::Get) },
			{ "PlayerState", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::PlayerState::Get) },
			{ "PlayerVariables", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::PlayerVariables::Get) },
			{ "SessionCooperativeDI", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::SessionCooperativeDI::Get) },
			{ "TPPCameraDI", reinterpret_cast<void*(*)()>(&EGSDK::GamePH::TPPCameraDI::Get) }
		};
		static const std::vector<std::pair<std::string_view, void*(*)()>> EngineClassAddrMap = {
			{ "CBulletPhysicsCharacter", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CBulletPhysicsCharacter::Get) },
			{ "CGSObject", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CGSObject::Get) },
			{ "CGSObject2", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CGSObject2::Get) },
			{ "CGame", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CGame::Get) },
			{ "CInput", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CInput::Get) },
			{ "CLevel", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CLevel::Get) },
			{ "CLevel2", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CLevel2::Get) },
			{ "CLobbySteam", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CLobbySteam::Get) },
			{ "CVideoSettings", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CVideoSettings::Get) },
			{ "CoPhysicsProperty", reinterpret_cast<void*(*)()>(&EGSDK::Engine::CoPhysicsProperty::Get) }
		};

		ImGui::Option disableVftableScanning { false };
#ifdef _DEBUG
		ImGui::Option enableDebuggingConsole { true };
#else
		ImGui::Option enableDebuggingConsole { false };
#endif

		static void RenderClassAddrPair(const std::pair<std::string_view, void*(*)()>* pair) {
			const float maxInputTextWidth = ImGui::CalcTextSize("0x0000000000000000").x;
			static std::string labelID{};
			labelID = "##DebugAddrInputText" + std::string(pair->first);

			std::stringstream ss{};
			if (pair->second())
				ss << "0x" << std::uppercase << std::hex << reinterpret_cast<DWORD64>(pair->second());
			else
				ss << "NULL";

			static std::string addrString{};
			addrString = ss.str();

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ((ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) / 2.0f));
			ImGui::Text(pair->first.data());

			ImGui::SameLine();

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ((ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) / 2.0f));
			ImGui::SetNextItemWidth(maxInputTextWidth);
			ImGui::PushStyleColor(ImGuiCol_Text, pair->second() ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
			ImGui::InputText(labelID.c_str(), const_cast<char*>(addrString.c_str()), strlen(addrString.c_str()), ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor();
		}

		Tab Tab::instance{};
		void Tab::Init() {}
		void Tab::Update() {}
		void Tab::Render() {
			ImGui::SeparatorText("Misc##Debug");
			if (ImGui::Checkbox("Disable Vftable Scanning", &disableVftableScanning, "Disables the vftable scanning for classes that are used in the game and used to validate a class in memory; this option is used for debugging purposes"))
				EGSDK::ClassHelpers::SetIsVftableScanningDisabled(disableVftableScanning.GetValue());
			ImGui::Checkbox("Enable Debugging Console *", &enableDebuggingConsole, "Enables EGameTools' debugging console that shows up when starting up the game; this option is used for debugging purposes");
			ImGui::SeparatorText("Class addresses##Debug");
			if (ImGui::CollapsingHeader("GamePH", ImGuiTreeNodeFlags_None)) {
				ImGui::Indent();
				for (auto& pair : GamePHClassAddrMap)
					RenderClassAddrPair(&pair);
				ImGui::Unindent();
			}
			if (ImGui::CollapsingHeader("Engine", ImGuiTreeNodeFlags_None)) {
				ImGui::Indent();
				for (auto& pair : EngineClassAddrMap)
					RenderClassAddrPair(&pair);
				ImGui::Unindent();
			}

			ImGui::SeparatorText("Dynamic RTTI Test##Debug");
			if (ImGui::Button("Dump Global RTTI")) {
				EGSDK::Engine::RTTIManager::Dump("RTTIDump.txt");
				SPDLOG_INFO("[RTTI Test] Global RTTI dumped to RTTIDump.txt");
			}

			if (ImGui::Button("Run Instance Discovery Test")) {
				auto rttiClass = EGSDK::Engine::RTTIManager::GetClass("CGame");
				if (rttiClass) {
					void* rttiInstance = EGSDK::Engine::RTTIManager::GetFirstInstance(rttiClass);
					void* manualInstance = EGSDK::Engine::CGame::Get();
					
					SPDLOG_INFO("[RTTI Test] Class: {}", rttiClass->GetName());
					SPDLOG_INFO("[RTTI Test] RTTI Discovery: {}", rttiInstance);
					SPDLOG_INFO("[RTTI Test] Manual Discovery: {}", manualInstance);
					
					if (rttiInstance == manualInstance && manualInstance != nullptr) {
						SPDLOG_INFO("[RTTI Test] SUCCESS: Instances match!");
					} else if (manualInstance != nullptr) {
						SPDLOG_WARN("[RTTI Test] WARNING: Instances do NOT match or RTTI failed.");
					} else {
						SPDLOG_ERROR("[RTTI Test] FAILED: Both instances are NULL.");
					}
				}
			}

			if (ImGui::Button("Run Player RTTI Test")) {
				auto rttiClass = EGSDK::Engine::RTTIManager::GetClass("PlayerDI");
				if (rttiClass) {
					SPDLOG_INFO("[RTTI Test] Found PlayerDI class: {}", rttiClass->GetName());
					
					auto player = EGSDK::GamePH::PlayerDI_PH::Get();
					if (player) {
						// PlayerDI_PH inherits from PlayerDI
						auto dynObj = EGSDK::Engine::DynamicObject(player, rttiClass);
						
						// Test m_DefaultFOV (float)
						float fov = dynObj.GetProperty<float>("m_DefaultFOV");
						SPDLOG_INFO("[RTTI Test] Initial DefaultFOV: {}", fov);
						
						// Modify it
						dynObj.SetProperty<float>("m_DefaultFOV", fov + 5.0f);
						SPDLOG_INFO("[RTTI Test] Modified DefaultFOV: {}", dynObj.GetProperty<float>("m_DefaultFOV"));
						
						// Revert
						dynObj.SetProperty<float>("m_DefaultFOV", fov);
						SPDLOG_INFO("[RTTI Test] RTTI verification completed successfully!");
					} else {
						SPDLOG_ERROR("[RTTI Test] Failed to find Player instance!");
					}
				} else {
					SPDLOG_ERROR("[RTTI Test] Failed to find PlayerDI class!");
				}
			}

			ImGui::Separator();
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "* Option requires game restart to apply");
		}
	}
}