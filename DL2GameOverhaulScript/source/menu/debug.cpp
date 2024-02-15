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
#include "..\game\GamePH\PlayerObjProperties.h"
#include "..\game\GamePH\PlayerState.h"
#include "..\game\GamePH\PlayerVariables.h"
#include "..\game\GamePH\SessionCooperativeDI.h"
#include "..\game\GamePH\TPPCameraDI.h"
#include "..\game\GamePH\TimeWeather\CSystem.h"
#include "..\game\GamePH\gen_TPPModel.h"

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
		Tab Tab::instance{};
		void Tab::Update() {}
		void Tab::Render() {
			ImGui::SeparatorText("Class addresses##Debug");
			if (ImGui::CollapsingHeader("GamePH", ImGuiTreeNodeFlags_None)) {
				ImGui::Indent();
				ImGui::Text("TimeWeather\\CSystem: 0x%p", GamePH::TimeWeather::CSystem::Get());
				ImGui::Text("DayNightCycle: 0x%p", GamePH::DayNightCycle::Get());
				ImGui::Text("FreeCamera: 0x%p", GamePH::FreeCamera::Get());
				ImGui::Text("GameDI_PH: 0x%p", GamePH::GameDI_PH::Get());
				ImGui::Text("GameDI_PH2: 0x%p", GamePH::GameDI_PH2::Get());
				ImGui::Text("gen_TPPModel: 0x%p", GamePH::gen_TPPModel::Get());
				ImGui::Text("LevelDI: 0x%p", GamePH::LevelDI::Get());
				ImGui::Text("LocalClientDI: 0x%p", GamePH::LocalClientDI::Get());
				ImGui::Text("LogicalPlayer: 0x%p", GamePH::LogicalPlayer::Get());
				ImGui::Text("PlayerDI_PH: 0x%p", GamePH::PlayerDI_PH::Get());
				ImGui::Text("PlayerHealthModule: 0x%p", GamePH::PlayerHealthModule::Get());
				ImGui::Text("PlayerObjProperties: 0x%p", GamePH::PlayerObjProperties::Get());
				ImGui::Text("PlayerState: 0x%p", GamePH::PlayerState::Get());
				ImGui::Text("PlayerVariables: 0x%p", GamePH::PlayerVariables::Get());
				ImGui::Text("SessionCooperativeDI: 0x%p", GamePH::SessionCooperativeDI::Get());
				ImGui::Text("TPPCameraDI: 0x%p", GamePH::TPPCameraDI::Get());
				ImGui::Unindent();
			}
			if (ImGui::CollapsingHeader("Engine", ImGuiTreeNodeFlags_None)) {
				ImGui::Indent();
				ImGui::Text("CBulletPhysicsCharacter: 0x%p", Engine::CBulletPhysicsCharacter::Get());
				ImGui::Text("CGSObject: 0x%p", Engine::CGSObject::Get());
				ImGui::Text("CGSObject2: 0x%p", Engine::CGSObject2::Get());
				ImGui::Text("CGame: 0x%p", Engine::CGame::Get());
				ImGui::Text("CInput: 0x%p", Engine::CInput::Get());
				ImGui::Text("CLevel: 0x%p", Engine::CLevel::Get());
				ImGui::Text("CLevel2: 0x%p", Engine::CLevel2::Get());
				ImGui::Text("CLobbySteam: 0x%p", Engine::CLobbySteam::Get());
				ImGui::Text("CVideoSettings: 0x%p", Engine::CVideoSettings::Get());
				ImGui::Text("CoPhysicsProperty: 0x%p", Engine::CoPhysicsProperty::Get());
				ImGui::Unindent();
			}
		}
	}
}