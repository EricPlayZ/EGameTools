#include <EGSDK\Offsets.h>
#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\Engine\CGame.h>
#include <EGSDK\Engine\CVideoSettings.h>
#include <EGSDK\GamePH\CoPlayerRestrictions.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\GamePH\GameDI_PH.h>
#include <EGSDK\GamePH\LocalClientDI.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\GamePH\PlayerState.h>
#include <EGSDK\GamePH\SessionCooperativeDI.h>
#include <EGSDK\ClassHelpers.h>

namespace EGSDK {
	bool OffsetManager::initialized = false;

	void OffsetManager::InitializeOffsetsAndPatterns() {
		if (initialized)
			return;

		AddOffsets(11200, {
			{ "OnPostUpdate", 0x378 },
			{ GetOffsetNameFromClassMember(&Engine::CBulletPhysicsCharacter::playerPos2), 0x880 },
			{ GetOffsetNameFromClassMember(&Engine::CBulletPhysicsCharacter::playerPos), 0x898 },
			{ GetOffsetNameFromClassMember(&Engine::CBulletPhysicsCharacter::playerDownwardVelocity), 0xC18 },
			{ GetOffsetNameFromClassMember(&Engine::CGame::pCLevel), 0x380 },
			{ GetOffsetNameFromClassMember(&Engine::CVideoSettings::extraFOV), 0x78 },

			{ GetOffsetNameFromClassMember(&GamePH::CoPlayerRestrictions::flags), 0x1C0 },
			{ GetOffsetNameFromClassMember(&GamePH::FreeCamera::enableSpeedMultiplier1), 0x4C },
			{ GetOffsetNameFromClassMember(&GamePH::FreeCamera::enableSpeedMultiplier2), 0x4D },
			{ GetOffsetNameFromClassMember(&GamePH::FreeCamera::speedMultiplier), 0x150 },
			//{ GetOffsetNameFromClassMember(&GamePH::FreeCamera::mouseSensitivityMultiplier), 0x1A0 },
			{ GetOffsetNameFromClassMember(&GamePH::GameDI_PH::blockPauseGameOnPlayerAfk), 0x830 },
			{ GetOffsetNameFromClassMember(&GamePH::GameDI_PH::pSessionCooperativeDI), 0xE8 },
			{ GetOffsetNameFromClassMember(&GamePH::LocalClientDI::pPlayerDI_PH), 0x88 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::pCoPhysicsProperty), 0xE8 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::pInventoryContainerDI), 0x550 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::nextPlayerOrientation), 0xC68 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::restrictionsEnabled), 0x2CF0 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::enableTPPModel1), 0x2DC1 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::enableTPPModel2), 0x2DC2 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerState::playerVariables), 0x280 },
			{ GetOffsetNameFromClassMember(&GamePH::SessionCooperativeDI::pLocalClientDI), 0xD10 },
			});
		AddOffsets(12001, {
			{ "OnPostUpdate", 0x3A8 },
			{ GetOffsetNameFromClassMember(&Engine::CBulletPhysicsCharacter::playerPos2), 0xCB8 },
			{ GetOffsetNameFromClassMember(&Engine::CBulletPhysicsCharacter::playerPos), 0xCD0 },
			{ GetOffsetNameFromClassMember(&Engine::CBulletPhysicsCharacter::playerDownwardVelocity), 0x1050 },
			{ GetOffsetNameFromClassMember(&Engine::CGame::pCLevel), 0x390 },
			{ GetOffsetNameFromClassMember(&Engine::CVideoSettings::extraFOV), 0x7C },

			{ GetOffsetNameFromClassMember(&GamePH::CoPlayerRestrictions::flags), 0x1F0 },
			{ GetOffsetNameFromClassMember(&GamePH::FreeCamera::enableSpeedMultiplier1), 0x42 },
			{ GetOffsetNameFromClassMember(&GamePH::FreeCamera::enableSpeedMultiplier2), 0x43 },
			{ GetOffsetNameFromClassMember(&GamePH::FreeCamera::speedMultiplier), 0x1CC },
			{ GetOffsetNameFromClassMember(&GamePH::GameDI_PH::blockPauseGameOnPlayerAfk), 0x910 },
			{ GetOffsetNameFromClassMember(&GamePH::GameDI_PH::pSessionCooperativeDI), 0x130 },
			{ GetOffsetNameFromClassMember(&GamePH::LocalClientDI::pPlayerDI_PH), 0x90 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::pCoPhysicsProperty), 0xF0 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::pInventoryContainerDI), 0x470 },
			//{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::pPlayerFppVis_PH), 0x420 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::nextPlayerOrientation), 0xB88 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::restrictionsEnabled), 0x3520 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::enableTPPModel1), 0x35E9 },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerDI_PH::enableTPPModel2), 0x35EA },
			{ GetOffsetNameFromClassMember(&GamePH::PlayerState::playerVariables), 0x300 },
			{ GetOffsetNameFromClassMember(&GamePH::SessionCooperativeDI::pLocalClientDI), 0xE08 },
			});

		AddPatterns(11200, {
			{ "LoadPlayerVars", { "40 55 53 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 33 FF", Utils::SigScan::PatternType::Address } },
			{ "PlayerState", { "48 8B 3D [?? ?? ?? ?? 4C 8B EA", Utils::SigScan::PatternType::RelativePointer } },
			{ "SaveGameCRCBoolCheck", { "FF 50 ?? [40 22 FB 0F 85 ?? ?? ?? ?? 0F B6 05 ?? ?? ?? ?? 48 8D 1D", Utils::SigScan::PatternType::Address } },
			{ "IsNotOutOfMapBounds", { "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 4C 8B C2", Utils::SigScan::PatternType::Address } },
			{ "IsNotOutOfMissionBounds", { "48 89 5C 24 ?? 57 48 83 EC ?? 4C 8B C2 48 8B F9", Utils::SigScan::PatternType::Address } },
			{ "PlaySoundEvent", { "4C 8B DC 49 89 5B ?? 49 89 73 ?? 57 48 81 EC ?? ?? ?? ?? 4C 8B 4C 24 ?? 48 8B F9 4D 8B D0 66 C7 84 24 ?? ?? ?? ?? ?? ?? 49 8B C1 66 C7 84 24", Utils::SigScan::PatternType::Address } },
			{ "GetPlayerRestrictionsFlags", { "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 48 8B DA 48 8B CA E8 ?? ?? ?? ?? 48 8B 4F", Utils::SigScan::PatternType::Address } },
			{ "EnablePlayerRestrictionsSubFunc", { "40 53 48 83 EC ?? 48 8B D9 48 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B CB 48 83 C4 ?? 5B E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 4C 24", Utils::SigScan::PatternType::Address } },
			{ "DisablePlayerRestrictionsSubFunc", { "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 83 EC ?? 0F B7 81", Utils::SigScan::PatternType::Address } },
			{ "HandlePlayerRestrictions", { "40 57 48 83 EC ?? 48 89 5C 24 ?? 48 8B F9 48 89 6C 24 ?? 0F B6 A9", Utils::SigScan::PatternType::Address } }
			});
		AddPatterns(12001, {
			{ "LoadPlayerVars", { "48 89 4C 24 ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B 8C 24", Utils::SigScan::PatternType::Address } },
			{ "PlayerState", { "48 8B 35 [?? ?? ?? ?? 4C 8B F2 48 8B F9", Utils::SigScan::PatternType::RelativePointer } },
			{ "SaveGameCRCBoolCheck", { "FF 50 ?? [40 22 DF 0F 85 ?? ?? ?? ?? 0F B6 05 ?? ?? ?? ?? 48 8D 3D", Utils::SigScan::PatternType::Address } },
			{ "IsNotOutOfMapBounds", { "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 4C 8B F9 48 85 D2", Utils::SigScan::PatternType::Address } },
			{ "IsNotOutOfMissionBounds", { "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 48 85 D2 74 ?? 48 8D 8A", Utils::SigScan::PatternType::Address } },
			{ "PlaySoundEvent", { "4C 8B DC 49 89 5B ?? 49 89 73 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 44 24 ?? 48 8B F9 48 8B DA", Utils::SigScan::PatternType::Address } },
			{ "GetPlayerRestrictionsFlags", { "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B D9 48 8B FA 48 8B CA E8 ?? ?? ?? ?? 48 8B 4B", Utils::SigScan::PatternType::Address } },
			{ "EnablePlayerRestrictionsSubFunc", { "40 53 48 83 EC ?? 48 8B D9 48 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B CB 48 83 C4 ?? 5B E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC 40 57", Utils::SigScan::PatternType::Address } },
			{ "DisablePlayerRestrictionsSubFunc", { "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 83 EC ?? 0F B6 81", Utils::SigScan::PatternType::Address } },
			{ "HandlePlayerRestrictions", { "40 57 48 83 EC ?? 48 89 5C 24 ?? 48 8B F9 48 89 74 24 ?? 0F B6 B1", Utils::SigScan::PatternType::Address } }
			});

		initialized = true;
	}
	DWORD OffsetManager::GetOffset(const std::string& offsetName) {
		if (!initialized)
			return 0;

		auto& offsets = GetOffsetsMap();
		if (offsets.find(Core::gameVer) != offsets.end() && offsets[Core::gameVer].find(offsetName) != offsets[Core::gameVer].end())
			return offsets[Core::gameVer][offsetName];

		SPDLOG_ERROR("Offset not found for key: \"{}\" in version: {}", offsetName, Core::gameVer);
		return 0;
	}
	Utils::SigScan::Pattern OffsetManager::GetPattern(const std::string& patternName) {
		if (!initialized)
			return {};

		auto& patterns = GetPatternsMap();
		if (patterns.find(Core::gameVer) != patterns.end() && patterns[Core::gameVer].find(patternName) != patterns[Core::gameVer].end())
			return patterns[Core::gameVer][patternName];

		SPDLOG_ERROR("Pattern not found for key: \"{}\" in version: {}", patternName, Core::gameVer);
		return {};
	}

	void OffsetManager::AddOffsets(DWORD gameVer, const std::unordered_map<std::string, DWORD>& offsets) {
		GetOffsetsMap()[gameVer] = offsets;
	}
	void OffsetManager::AddPatterns(DWORD gameVer, const std::unordered_map<std::string, Utils::SigScan::Pattern>& patterns) {
		GetPatternsMap()[gameVer] = patterns;
	}

	std::unordered_map<DWORD, std::unordered_map<std::string, DWORD>>& OffsetManager::GetOffsetsMap() {
		static std::unordered_map<DWORD, std::unordered_map<std::string, DWORD>> offsetsMap;
		return offsetsMap;
	}
	std::unordered_map<DWORD, std::unordered_map<std::string, Utils::SigScan::Pattern>>& OffsetManager::GetPatternsMap() {
		static std::unordered_map<DWORD, std::unordered_map<std::string, Utils::SigScan::Pattern>> patternsMap;
		return patternsMap;
	}
}