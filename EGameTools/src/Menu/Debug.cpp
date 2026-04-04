#include <ImGui\imguiex.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Offsets.h>
#include <EGT\ImGui_impl\Win32_impl.h>
#include <EGT\Menu\Debug.h>

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include <EGSDK\GamePH\CoPlayerRestrictions.h>
#include <EGSDK\GamePH\DayNightCycle.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\GamePH\GameDI_PH.h>
#include <EGSDK\GamePH\GameDI_PH2.h>
#include <EGSDK\GamePH\InventoryMoney.h>
#include <EGSDK\GamePH\ItemDescWithContext.h>
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

		namespace {
#define STATIC_BUF_OFF(Class, Field, N) (static_cast<uint64_t>(offsetof(Class, Field)) + (static_cast<uint64_t>(N)))

			template<typename ParentT, typename FieldT>
			uint64_t DynOff(FieldT ParentT::* member) {
				return EGSDK::OffsetManager::GetOffset(EGSDK::ClassHelpers::GetOffsetNameFromClassMember(member));
			}

			struct UnionFieldEntry {
				const char* label;
				const char* classShortName;
				void* (*getBase)();
				uint64_t (*getFieldOffset)();
			};

			static const std::vector<UnionFieldEntry> kGamePHUnionFields = {
				{ "TimeWeather\\CSystem::blendTime", "TimeWeather\\CSystem", []() -> void* { return EGSDK::GamePH::TimeWeather::CSystem::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::TimeWeather::CSystem, blendTime, 0x10); } },
				{ "TimeWeather\\CSystem::blendTime2", "TimeWeather\\CSystem", []() -> void* { return EGSDK::GamePH::TimeWeather::CSystem::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::TimeWeather::CSystem, blendTime2, 0x14); } },
				{ "TimeWeather\\CSystem::currentSubSystem", "TimeWeather\\CSystem", []() -> void* { return EGSDK::GamePH::TimeWeather::CSystem::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::TimeWeather::CSystem, currentSubSystem, 0x70); } },
				{ "TimeWeather\\CSystem::nextSubSystem", "TimeWeather\\CSystem", []() -> void* { return EGSDK::GamePH::TimeWeather::CSystem::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::TimeWeather::CSystem, nextSubSystem, 0x78); } },
				{ "TimeWeather\\CSystem::lastSubSystem", "TimeWeather\\CSystem", []() -> void* { return EGSDK::GamePH::TimeWeather::CSystem::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::TimeWeather::CSystem, lastSubSystem, 0x80); } },

				{ "CoPlayerRestrictions::flags", "CoPlayerRestrictions", []() -> void* { return EGSDK::GamePH::CoPlayerRestrictions::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::CoPlayerRestrictions::flags); } },

				{ "DayNightCycle::time1", "DayNightCycle", []() -> void* { return EGSDK::GamePH::DayNightCycle::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::DayNightCycle, time1, 0x10); } },
				{ "DayNightCycle::time2", "DayNightCycle", []() -> void* { return EGSDK::GamePH::DayNightCycle::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::DayNightCycle, time2, 0x20); } },
				{ "DayNightCycle::time3", "DayNightCycle", []() -> void* { return EGSDK::GamePH::DayNightCycle::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::DayNightCycle, time3, 0x5C); } },

				{ "FreeCamera::pCoBaseCameraProxy", "FreeCamera", []() -> void* { return EGSDK::GamePH::FreeCamera::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::FreeCamera, pCoBaseCameraProxy, 0x18); } },
				{ "FreeCamera::pCBaseCamera", "FreeCamera", []() -> void* { return EGSDK::GamePH::FreeCamera::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::FreeCamera, pCBaseCamera, 0x38); } },
				{ "FreeCamera::enableSpeedMultiplier1", "FreeCamera", []() -> void* { return EGSDK::GamePH::FreeCamera::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::FreeCamera::enableSpeedMultiplier1); } },
				{ "FreeCamera::enableSpeedMultiplier2", "FreeCamera", []() -> void* { return EGSDK::GamePH::FreeCamera::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::FreeCamera::enableSpeedMultiplier2); } },
				{ "FreeCamera::speedMultiplier", "FreeCamera", []() -> void* { return EGSDK::GamePH::FreeCamera::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::FreeCamera::speedMultiplier); } },

				{ "GameDI_PH::blockPauseGameOnPlayerAfk", "GameDI_PH", []() -> void* { return EGSDK::GamePH::GameDI_PH::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::GameDI_PH::blockPauseGameOnPlayerAfk); } },
				{ "GameDI_PH::pSessionCooperativeDI", "GameDI_PH", []() -> void* { return EGSDK::GamePH::GameDI_PH::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::GameDI_PH::pSessionCooperativeDI); } },

				{ "LocalClientDI::pPlayerDI_PH", "LocalClientDI", []() -> void* { return EGSDK::GamePH::LocalClientDI::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::LocalClientDI::pPlayerDI_PH); } },

				{ "PlayerDI_PH::pCoPhysicsProperty", "PlayerDI_PH", []() -> void* { return EGSDK::GamePH::PlayerDI_PH::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::PlayerDI_PH::pCoPhysicsProperty); } },
				{ "PlayerDI_PH::pInventoryContainerDI", "PlayerDI_PH", []() -> void* { return EGSDK::GamePH::PlayerDI_PH::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::PlayerDI_PH::pInventoryContainerDI); } },
				{ "PlayerDI_PH::nextPlayerOrientation", "PlayerDI_PH", []() -> void* { return EGSDK::GamePH::PlayerDI_PH::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::PlayerDI_PH::nextPlayerOrientation); } },
				{ "PlayerDI_PH::restrictionsEnabled", "PlayerDI_PH", []() -> void* { return EGSDK::GamePH::PlayerDI_PH::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::PlayerDI_PH::restrictionsEnabled); } },
				{ "PlayerDI_PH::enableTPPModel1", "PlayerDI_PH", []() -> void* { return EGSDK::GamePH::PlayerDI_PH::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::PlayerDI_PH::enableTPPModel1); } },
				{ "PlayerDI_PH::enableTPPModel2", "PlayerDI_PH", []() -> void* { return EGSDK::GamePH::PlayerDI_PH::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::PlayerDI_PH::enableTPPModel2); } },

				{ "PlayerHealthModule::pPlayerDI_PH", "PlayerHealthModule", []() -> void* { return EGSDK::GamePH::PlayerHealthModule::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::PlayerHealthModule, pPlayerDI_PH, 0x8); } },
				{ "PlayerHealthModule::health", "PlayerHealthModule", []() -> void* { return EGSDK::GamePH::PlayerHealthModule::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::PlayerHealthModule, health, 0x2C); } },
				{ "PlayerHealthModule::maxHealth", "PlayerHealthModule", []() -> void* { return EGSDK::GamePH::PlayerHealthModule::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::PlayerHealthModule, maxHealth, 0x3C); } },

				{ "PlayerInfectionModule::pPlayerDI_PH", "PlayerInfectionModule", []() -> void* { return EGSDK::GamePH::PlayerInfectionModule::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::PlayerInfectionModule, pPlayerDI_PH, 0x8); } },
				{ "PlayerInfectionModule::maxImmunity", "PlayerInfectionModule", []() -> void* { return EGSDK::GamePH::PlayerInfectionModule::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::PlayerInfectionModule, maxImmunity, 0x20); } },
				{ "PlayerInfectionModule::immunity", "PlayerInfectionModule", []() -> void* { return EGSDK::GamePH::PlayerInfectionModule::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::PlayerInfectionModule, immunity, 0x2C); } },
				{ "PlayerInfectionModule::nightrunnerTimer", "PlayerInfectionModule", []() -> void* { return EGSDK::GamePH::PlayerInfectionModule::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::PlayerInfectionModule, nightrunnerTimer, 0x98); } },

				{ "PlayerState::playerVariables", "PlayerState", []() -> void* { return EGSDK::GamePH::PlayerState::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::PlayerState::playerVariables); } },

				{ "SessionCooperativeDI::pLocalClientDI", "SessionCooperativeDI", []() -> void* { return EGSDK::GamePH::SessionCooperativeDI::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::GamePH::SessionCooperativeDI::pLocalClientDI); } },

				{ "InventoryMoney::oldWorldMoney", "InventoryMoney", []() -> void* {
					auto* player = EGSDK::GamePH::PlayerDI_PH::Get();
					if (!player)
						return nullptr;
					auto* container = player->GetInventoryContainer();
					return container ? reinterpret_cast<void*>(container->GetInventoryMoney(0)) : nullptr;
				  },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::InventoryMoney, oldWorldMoney, 0x38); } },
				{ "ItemDescWithContext::weaponDurability", "ItemDescWithContext", []() -> void* {
					auto* player = EGSDK::GamePH::PlayerDI_PH::Get();
					if (!player)
						return nullptr;
					auto* item = player->GetCurrentWeapon(0);
					return item ? reinterpret_cast<void*>(item->GetItemDescCtx()) : nullptr;
				  },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::GamePH::ItemDescWithContext, weaponDurability, 0xA8); } },
			};

			static const std::vector<UnionFieldEntry> kEngineUnionFields = {
				{ "CLobbySteam::pCGame", "CLobbySteam", []() -> void* { return EGSDK::Engine::CLobbySteam::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CLobbySteam, pCGame, 0xF8); } },

				{ "CGame::pGameDI_PH", "CGame", []() -> void* { return EGSDK::Engine::CGame::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CGame, pGameDI_PH, 0x8); } },
				{ "CGame::pCVideoSettings", "CGame", []() -> void* { return EGSDK::Engine::CGame::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CGame, pCVideoSettings, 0x28); } },
				{ "CGame::pCLevel", "CGame", []() -> void* { return EGSDK::Engine::CGame::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::Engine::CGame::pCLevel); } },

				{ "CVideoSettings::extraFOV", "CVideoSettings", []() -> void* { return EGSDK::Engine::CVideoSettings::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::Engine::CVideoSettings::extraFOV); } },

				{ "CBulletPhysicsCharacter::playerPos", "CBulletPhysicsCharacter", []() -> void* { return EGSDK::Engine::CBulletPhysicsCharacter::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::Engine::CBulletPhysicsCharacter::playerPos); } },
				{ "CBulletPhysicsCharacter::playerDownwardVelocity", "CBulletPhysicsCharacter", []() -> void* { return EGSDK::Engine::CBulletPhysicsCharacter::Get(); },
				  []() -> uint64_t { return DynOff(&EGSDK::Engine::CBulletPhysicsCharacter::playerDownwardVelocity); } },

				{ "CLevel::pLevelDI", "CLevel", []() -> void* { return EGSDK::Engine::CLevel::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CLevel, pLevelDI, 0x20); } },
				{ "CLevel::pCGSObject", "CLevel", []() -> void* { return EGSDK::Engine::CLevel::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CLevel, pCGSObject, 0x30); } },

				{ "CGSObject::pCLevel2", "CGSObject", []() -> void* { return EGSDK::Engine::CGSObject::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CGSObject, pCLevel2, 0x48); } },

				{ "CLevel2::pCGSObject2", "CLevel2", []() -> void* { return EGSDK::Engine::CLevel2::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CLevel2, pCGSObject2, 0x28); } },

				{ "CGSObject2::pLogicalPlayer", "CGSObject2", []() -> void* { return EGSDK::Engine::CGSObject2::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CGSObject2, pLogicalPlayer, 0x20); } },

				{ "CoPhysicsProperty::pCBulletPhysicsCharacter", "CoPhysicsProperty", []() -> void* { return EGSDK::Engine::CoPhysicsProperty::Get(); },
				  []() -> uint64_t { return STATIC_BUF_OFF(EGSDK::Engine::CoPhysicsProperty, pCBulletPhysicsCharacter, 0x20); } },
			};

#undef STATIC_BUF_OFF

			static void RenderHexPtrRow(const char* label, const void* ptr) {
				const float maxInputTextWidth = ImGui::CalcTextSize("0x0000000000000000").x;
				const std::string labelID = std::string("##InvDbgHex") + label;

				std::stringstream ss{};
				if (ptr)
					ss << "0x" << std::uppercase << std::hex << reinterpret_cast<uint64_t>(ptr);
				else
					ss << "NULL";

				static std::string addrString{};
				addrString = ss.str();

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ((ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) / 2.0f));
				ImGui::Text("%s", label);
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ((ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) / 2.0f));
				ImGui::SetNextItemWidth(maxInputTextWidth);
				ImGui::PushStyleColor(ImGuiCol_Text, ptr ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
				ImGui::InputText(labelID.c_str(), const_cast<char*>(addrString.c_str()), addrString.size() + 1, ImGuiInputTextFlags_ReadOnly);
				ImGui::PopStyleColor();
			}

			static void RenderUnionFieldRow(const char* label, const char* classShortName, void* basePtr, uint64_t fieldOffset) {
				const float maxInputTextWidth = ImGui::CalcTextSize("0x0000000000000000  ->  TimeWeather\\CSystem+0xFFFFFFFF").x;
				static std::string labelID{};
				labelID = "##UnionFld" + std::string(label);

				std::stringstream ss{};
				ss << classShortName << "+0x" << std::uppercase << std::hex << fieldOffset;
				if (basePtr)
					ss << "  ->  0x" << std::uppercase << std::hex << (reinterpret_cast<uint64_t>(basePtr) + fieldOffset);

				static std::string line{};
				line = ss.str();

				const ImU32 col = !basePtr ? IM_COL32(255, 0, 0, 255)
					: (fieldOffset == 0 ? IM_COL32(255, 200, 0, 255) : IM_COL32(0, 255, 0, 255));

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ((ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) / 2.0f));
				ImGui::Text("%s", label);
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ((ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) / 2.0f));
				ImGui::SetNextItemWidth(maxInputTextWidth);
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				ImGui::InputText(labelID.c_str(), const_cast<char*>(line.c_str()), line.size() + 1, ImGuiInputTextFlags_ReadOnly);
				ImGui::PopStyleColor();
			}

			static void RenderUnionFieldList(const std::vector<UnionFieldEntry>& entries) {
				for (const auto& e : entries) {
					void* b = e.getBase();
					uint64_t off = e.getFieldOffset();
					RenderUnionFieldRow(e.label, e.classShortName, b, off);
				}
			}

			static void RenderInventoryItemContextOffsetProbe() {
				auto* player = EGSDK::GamePH::PlayerDI_PH::Get();
				auto* invContainer = player ? player->GetInventoryContainer() : nullptr;
				auto* invMoney = invContainer ? invContainer->GetInventoryMoney(0) : nullptr;
				auto* invItem = player ? player->GetCurrentWeapon(0) : nullptr;
				void* itemCtx = invItem ? invItem->GetItemDescCtx() : nullptr;
				constexpr uint64_t kItemToDescCtxSdkOffset = 0x40;
				void* itemCtxByFixedOffset = invItem ? reinterpret_cast<void*>(reinterpret_cast<uint64_t>(invItem) + kItemToDescCtxSdkOffset) : nullptr;

				ImGui::TextUnformatted("Pointer chain (PlayerDI_PH -> container / weapon). Equip a weapon to populate item context.");
				RenderHexPtrRow("PlayerDI_PH", player);
				RenderHexPtrRow("InventoryContainerDI", invContainer);
				RenderHexPtrRow("InventoryMoney (GetInventoryMoney(0))", invMoney);
				RenderHexPtrRow("InventoryItem (GetCurrentWeapon(0))", invItem);
				RenderHexPtrRow("ItemDescWithContext (GetItemDescCtx)", itemCtx);

				ImGui::SeparatorText("ItemDescWithContext base offset##InvDbg");
				ImGui::TextUnformatted("SDK hardcodes InventoryItem+0x40 in InventoryItem.cpp; compare with GetItemDescCtx() after vtable checks.");
				RenderHexPtrRow("InventoryItem + 0x40 (no vtable check)", itemCtxByFixedOffset);
				if (invItem && itemCtx && itemCtxByFixedOffset != itemCtx) {
					ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 180, 0, 255)),
						"Raw +0x40 differs from GetItemDescCtx — SafeGetter may have rejected the pointer, or offset changed.");
				} else if (invItem && itemCtx && itemCtxByFixedOffset == itemCtx) {
					ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 200, 100, 255)), "+0x40 matches GetItemDescCtx.");
				}

				ImGui::SeparatorText("Union fields (StaticBuffer)##InvDbg");
				ImGui::PushID("InvDbgUnionRows");
				RenderUnionFieldRow("InventoryMoney::oldWorldMoney", "InventoryMoney", invMoney,
					static_cast<uint64_t>(offsetof(EGSDK::GamePH::InventoryMoney, oldWorldMoney)) + 0x38);
				RenderUnionFieldRow("ItemDescWithContext::weaponDurability", "ItemDescWithContext", itemCtx,
					static_cast<uint64_t>(offsetof(EGSDK::GamePH::ItemDescWithContext, weaponDurability)) + 0xA8);
				ImGui::PopID();
			}
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
			ImGui::SeparatorText("Union field addresses (Class+offset)##Debug");
			ImGui::TextUnformatted("Read-only: ClassName+field offset from OffsetManager / StaticBuffer; when the instance exists, resolved address is appended.");
			if (ImGui::CollapsingHeader("GamePH##UnionFields", ImGuiTreeNodeFlags_None)) {
				ImGui::Indent();
				RenderUnionFieldList(kGamePHUnionFields);
				ImGui::Unindent();
			}
			if (ImGui::CollapsingHeader("Engine##UnionFields", ImGuiTreeNodeFlags_None)) {
				ImGui::Indent();
				RenderUnionFieldList(kEngineUnionFields);
				ImGui::Unindent();
			}
			ImGui::SeparatorText("InventoryMoney / ItemDescWithContext (live)##Debug");
			if (ImGui::CollapsingHeader("Pointer chain & offset probe##InvDbg", ImGuiTreeNodeFlags_None)) {
				ImGui::Indent();
				RenderInventoryItemContextOffsetProbe();
				ImGui::Unindent();
			}
			ImGui::Separator();
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "* Option requires game restart to apply");
		}
	}
}