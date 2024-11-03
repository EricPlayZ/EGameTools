#include <pch.h>
#include "debug.h"

#include "..\game\GamePH\DayNightCycle.h"
#include "..\game\GamePH\FreeCamera.h"
#include "..\game\GamePH\GameDI_PH.h"
#include "..\game\GamePH\GameDI_PH2.h"
#include "..\game\GamePH\LevelDI.h"
#include "..\game\GamePH\LocalClientDI.h"
#include "..\game\GamePH\LogicalPlayer.h"
#include "..\game\GamePH\PlayerDI_PH.h"
#include "..\game\GamePH\PlayerHealthModule.h"
#include "..\game\GamePH\PlayerState.h"
#include "..\game\GamePH\PlayerVariables.h"
#include "..\game\GamePH\SessionCooperativeDI.h"
#include "..\game\GamePH\TPPCameraDI.h"
#include "..\game\GamePH\TimeWeather\CSystem.h"

#include "..\game\Engine\CBulletPhysicsCharacter.h"
#include "..\game\Engine\CGSObject.h"
#include "..\game\Engine\CGSObject2.h"
#include "..\game\Engine\CGame.h"
#include "..\game\Engine\CInput.h"
#include "..\game\Engine\CLevel.h"
#include "..\game\Engine\CLevel2.h"
#include "..\game\Engine\CLobbySteam.h"
#include "..\game\Engine\CVideoSettings.h"
#include "..\game\Engine\CoPhysicsProperty.h"

namespace Menu {
	namespace Debug {
		static const std::vector<std::pair<std::string_view, LPVOID(*)()>> GamePHClassAddrMap = {
			{ "TimeWeather\\CSystem", reinterpret_cast<LPVOID(*)()>(&GamePH::TimeWeather::CSystem::Get)},
			{ "DayNightCycle", reinterpret_cast<LPVOID(*)()>(&GamePH::DayNightCycle::Get) },
			{ "FreeCamera", reinterpret_cast<LPVOID(*)()>(&GamePH::FreeCamera::Get) },
			{ "GameDI_PH", reinterpret_cast<LPVOID(*)()>(&GamePH::GameDI_PH::Get) },
			{ "GameDI_PH2", reinterpret_cast<LPVOID(*)()>(&GamePH::GameDI_PH2::Get) },
			{ "LevelDI", reinterpret_cast<LPVOID(*)()>(&GamePH::LevelDI::Get) },
			{ "LocalClientDI", reinterpret_cast<LPVOID(*)()>(&GamePH::LocalClientDI::Get) },
			{ "LogicalPlayer", reinterpret_cast<LPVOID(*)()>(&GamePH::LogicalPlayer::Get) },
			{ "PlayerDI_PH", reinterpret_cast<LPVOID(*)()>(&GamePH::PlayerDI_PH::Get) },
			{ "PlayerHealthModule", reinterpret_cast<LPVOID(*)()>(&GamePH::PlayerHealthModule::Get) },
			{ "PlayerState", reinterpret_cast<LPVOID(*)()>(&GamePH::PlayerState::Get) },
			{ "PlayerVariables", reinterpret_cast<LPVOID(*)()>(&GamePH::PlayerVariables::Get) },
			{ "SessionCooperativeDI", reinterpret_cast<LPVOID(*)()>(&GamePH::SessionCooperativeDI::Get) },
			{ "TPPCameraDI", reinterpret_cast<LPVOID(*)()>(&GamePH::TPPCameraDI::Get) }
		};
		static const std::vector<std::pair<std::string_view, LPVOID(*)()>> EngineClassAddrMap = {
			{ "CBulletPhysicsCharacter", reinterpret_cast<LPVOID(*)()>(&Engine::CBulletPhysicsCharacter::Get) },
			{ "CGSObject", reinterpret_cast<LPVOID(*)()>(&Engine::CGSObject::Get) },
			{ "CGSObject2", reinterpret_cast<LPVOID(*)()>(&Engine::CGSObject2::Get) },
			{ "CGame", reinterpret_cast<LPVOID(*)()>(&Engine::CGame::Get) },
			{ "CInput", reinterpret_cast<LPVOID(*)()>(&Engine::CInput::Get) },
			{ "CLevel", reinterpret_cast<LPVOID(*)()>(&Engine::CLevel::Get) },
			{ "CLevel2", reinterpret_cast<LPVOID(*)()>(&Engine::CLevel2::Get) },
			{ "CLobbySteam", reinterpret_cast<LPVOID(*)()>(&Engine::CLobbySteam::Get) },
			{ "CVideoSettings", reinterpret_cast<LPVOID(*)()>(&Engine::CVideoSettings::Get) },
			{ "CoPhysicsProperty", reinterpret_cast<LPVOID(*)()>(&Engine::CoPhysicsProperty::Get) }
		};

		static void RenderClassAddrPair(const std::pair<std::string_view, LPVOID(*)()>* pair) {
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
		void Tab::Update() {}
		void Tab::Render() {
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
		}
	}
}