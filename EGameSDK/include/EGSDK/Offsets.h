#pragma once
#include <EGSDK\Core\Core.h>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Utils\RTTI.h>
#include <EGSDK\Utils\Sigscan.h>
#include <EGSDK\Exports.h>

namespace EGSDK {
	#define AddPattern(name, moduleName, pattern, type, retType)\
	static retType Get_## name () {\
		static retType name = NULL;\
		static int i = 0;\
		if (!Utils::Memory::IsBadReadPtr(name) || !GetModuleHandleA(moduleName) || i >= 50) return name;\
		i++;\
		return name=reinterpret_cast<retType>(Utils::SigScan::PatternScanner::FindPattern(moduleName, {pattern, type}));\
	} 
	#define AddDynamicPattern(name, moduleName, retType)\
	static retType Get_## name () {\
		static retType name = NULL;\
		static int i = 0;\
		static Utils::SigScan::Pattern pattern = OffsetManager::GetPattern(#name);\
		if (!pattern.pattern) pattern = OffsetManager::GetPattern(#name);\
		if (!Utils::Memory::IsBadReadPtr(name) || !GetModuleHandleA(moduleName) || !pattern.pattern || i >= 50) return name;\
		i++;\
		return name=reinterpret_cast<retType>(Utils::SigScan::PatternScanner::FindPattern(moduleName, pattern));\
	} 

	#define AddStaticOffset(name, off)\
	static uint64_t Get_## name () {\
		static uint64_t name = 0;\
		if (name) return name; \
		return name=static_cast<uint64_t>(off);\
	} 
	#define AddStaticOffset2(name, moduleName, off) \
	static uint64_t Get_## name () {\
		static uint64_t name = 0;\
		if (!Utils::Memory::IsBadReadPtr(name)) return name;\
		return name=reinterpret_cast<uint64_t>(GetModuleHandleA(moduleName)) + static_cast<uint64_t>(off);\
	}

	class EGameSDK_API OffsetManager {
	public:
		static bool initialized;

		static void InitializeOffsetsAndPatterns();
		static DWORD GetOffset(const std::string& offsetName);
		static Utils::SigScan::Pattern GetPattern(const std::string& patternName);

		// Input related
		AddPattern(CInput, "engine_x64_rwdi.dll", "48 8B 0D [?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 01 84 D2", Utils::SigScan::PatternType::RelativePointer, uint64_t**) // g_CInput
		AddPattern(CDIDev_GetDeviceData, "dinput8.dll", "40 53 57 48 83 EC ?? 48 8B 01", Utils::SigScan::PatternType::Address, void*)

		// Rendering related
		AddPattern(DXPresent, "sl.interposer.dll", "48 89 5C 24 ?? 48 89 74 24 ?? 57 41 56", Utils::SigScan::PatternType::Address, void*)
		AddPattern(DX12Present1, "sl.interposer.dll", "48 89 5C 24 ?? 48 89 74 24 ? 48 89 7C 24 ?? 41 55 41 56 41 57 48 83 EC ?? 49 8B F1 45 8B F0", Utils::SigScan::PatternType::Address, void*)
		AddPattern(DXResizeBuffers, "sl.interposer.dll", "48 83 EC ?? 48 89 4C 24 ?? 8B 44 24", Utils::SigScan::PatternType::Address, void*)
		AddPattern(DX12ResizeBuffers1, "sl.interposer.dll", "48 83 EC ?? 48 89 4C 24 ?? 48 8B 84 24", Utils::SigScan::PatternType::Address, void*)
		AddPattern(DX12ExecuteCommandLists, "sl.interposer.dll", "48 8B C4 56 57 41 54", Utils::SigScan::PatternType::Address, void*)

		// Player vars related
		AddDynamicPattern(LoadPlayerVars, "gamedll_ph_x64_rwdi.dll", void*)
		AddDynamicPattern(PlayerState, "gamedll_ph_x64_rwdi.dll", void*)

		// Game related
		AddStaticOffset(gameDI_PH2_offset, 0x28)
		AddStaticOffset(allowVelocityMod_offset, 0x5C)
		AddStaticOffset(disableHeadCorrection_offset, 0x108)
		AddPattern(CLobbySteam, "engine_x64_rwdi.dll", "48 8B 05 [?? ?? ?? ?? 48 85 C0 74 ?? 48 83 C0", Utils::SigScan::PatternType::RelativePointer, void*)
		//AddPattern(g_PlayerDI_PH, "gamedll_ph_x64_rwdi.dll", "48 89 0D [?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0", Utils::SigScan::PatternType::RelativePointer, void*) // also PlayerObjProperties
		AddPattern(DayNightCycle, "gamedll_ph_x64_rwdi.dll", "48 8B 0D [?? ?? ?? ?? 48 85 C9 74 ?? E8 ?? ?? ?? ?? 84 C0 74 ?? B0 ?? 48 83 C4 ?? C3 32 C0", Utils::SigScan::PatternType::RelativePointer, void*) // g_DayNightCycle
		//AddPattern(CameraFPPDI, "gamedll_ph_x64_rwdi.dll", "48 89 05 [?? ?? ?? ?? 40 84 FF", Utils::SigScan::PatternType::RelativePointer, uint64_t*)
		AddPattern(FreeCamera, "gamedll_ph_x64_rwdi.dll", "48 89 05 [?? ?? ?? ?? 48 89 4C 24", Utils::SigScan::PatternType::RelativePointer, uint64_t*) // g_FreeCamera
		AddDynamicPattern(SaveGameCRCBoolCheck, "gamedll_ph_x64_rwdi.dll", void*)

		// Functions
		AddPattern(MountDataPaks, "engine_x64_rwdi.dll", "4C 8B DC 4D 89 4B ?? 45 89 43 ?? 89 54 24 ?? 49 89 4B", Utils::SigScan::PatternType::Address, void*)
		AddPattern(MoveCameraFromForwardUpPos, "engine_x64_rwdi.dll", "48 89 5C 24 ?? 57 48 83 EC ?? 49 8B C1 48 8B F9", Utils::SigScan::PatternType::Address, void*)
		AddPattern(CalculateFreeCamCollision, "gamedll_ph_x64_rwdi.dll", "48 8B C4 55 53 56 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 B9", Utils::SigScan::PatternType::Address, void*)
		AddPattern(AllowCameraMovement, "gamedll_ph_x64_rwdi.dll", "89 91 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC 48 8B C4 55 56", Utils::SigScan::PatternType::Address, void*)
		AddPattern(CreatePlayerHealthModule, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 4C 8B F1 E8 ?? ?? ?? ?? 48 8D 05", Utils::SigScan::PatternType::Address, void*)
		AddPattern(CreatePlayerInfectionModule, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B D9 E8 ?? ?? ?? ?? 33 FF 66 C7 44 24 ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 03", Utils::SigScan::PatternType::Address, void*)
		AddPattern(LifeSetHealth, "gamedll_ph_x64_rwdi.dll", "F3 0F 11 49 ?? F3 0F 11 49 ?? F3 0F 11 49 ?? C3", Utils::SigScan::PatternType::Address, void*)
		AddPattern(TogglePhotoMode1, "gamedll_ph_x64_rwdi.dll", "48 83 EC ?? 38 91 ?? ?? ?? ?? 0F 84", Utils::SigScan::PatternType::Address, void*)
		AddPattern(TogglePhotoMode2, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B D9 41 0F B6 F8 48 8B 0D", Utils::SigScan::PatternType::Address, void*)
		AddPattern(ShowTPPModelFunc3, "gamedll_ph_x64_rwdi.dll", "48 83 EC ?? 38 91 ?? ?? ?? ?? 74 ?? 88 91", Utils::SigScan::PatternType::Address, void*)
		AddDynamicPattern(IsNotOutOfMapBounds, "gamedll_ph_x64_rwdi.dll", void*)
		AddDynamicPattern(IsNotOutOfMissionBounds, "gamedll_ph_x64_rwdi.dll", void*)
		AddPattern(ReloadJumps, "gamedll_ph_x64_rwdi.dll", "48 83 EC ?? E8 ?? ?? ?? ?? 48 8D 15", Utils::SigScan::PatternType::Address, void*)
		AddDynamicPattern(PlaySoundEvent, "gamedll_ph_x64_rwdi.dll", void*)
		AddPattern(CalculateFallHeight, "gamedll_ph_x64_rwdi.dll", "40 55 56 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 44 0F 29 9C 24", Utils::SigScan::PatternType::Address, void*)
		AddPattern(PlayerHealthModuleKillPlayer, "gamedll_ph_x64_rwdi.dll", "40 53 48 83 EC ?? 48 8B 01 48 8B D9 FF 90 ?? ?? ?? ?? 84 C0 74 ?? 48 8B 4B ?? 48 81 C1 ?? ?? ?? ?? 48 8B 01 FF 50", Utils::SigScan::PatternType::Address, void*)
		AddPattern(ReadPlayerJumpParams, "gamedll_ph_x64_rwdi.dll", "40 55 56 57 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 4C 8B B5", Utils::SigScan::PatternType::Address, void*)
		AddPattern(PlayerGetCurrentWeapon, "gamedll_ph_x64_rwdi.dll", "8B C2 48 8D 14 80 48 83 BC D1 ?? ?? ?? ?? ?? 74 ?? 48 8B 84 D1 ?? ?? ?? ?? C3 33 C0 C3 CC CC CC 8B C2", Utils::SigScan::PatternType::Address, void*)
		AddPattern(PlayerGetInventoryMoney, "gamedll_ph_x64_rwdi.dll", "8B C2 48 8B 44 C1", Utils::SigScan::PatternType::Address, void*)
		AddPattern(HandleInventoryItemsAmount, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 8B 29", Utils::SigScan::PatternType::Address, void*)
		AddPattern(SetNewWaypointLocation, "gamedll_ph_x64_rwdi.dll", "85 D2 78 ?? 48 89 74 24 ?? 57 48 83 EC ?? 49 8B F8", Utils::SigScan::PatternType::Address, void*)
		AddPattern(SetNewWaypointLocationWaypointIsSetBoolInstr, "gamedll_ph_x64_rwdi.dll", "C6 84 33 [?? ?? ?? ?? 01 48 8B 5C 24", Utils::SigScan::PatternType::Address, DWORD*)
		AddDynamicPattern(GetPlayerRestrictionsFlags, "gamedll_ph_x64_rwdi.dll", void*)
		AddPattern(mEnablePlayerRestrictions, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 74 ?? 48 8D 54 24", Utils::SigScan::PatternType::Address, void*)
		AddPattern(mDisablePlayerRestrictions, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 56 48 83 EC ?? 48 8B F1 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 74 ?? 48 8D 48 ?? 48 89 7C 24 ?? FF 15 ?? ?? ?? ?? 84 C0 75 ?? 48 8D 4B ?? FF 15 ?? ?? ?? ?? 84 C0 75 ?? 48 8D 96", Utils::SigScan::PatternType::Address, void*)
		AddDynamicPattern(EnablePlayerRestrictionsSubFunc, "gamedll_ph_x64_rwdi.dll", void*)
		AddDynamicPattern(DisablePlayerRestrictionsSubFunc, "gamedll_ph_x64_rwdi.dll", void*)
		AddDynamicPattern(HandlePlayerRestrictions, "gamedll_ph_x64_rwdi.dll", void*) // sub func of m_EnableRestrictions sub funcs and m_DisableRestrictions sub funcs
		AddPattern(SetIsInCoSafeZone, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 74 ?? E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4", Utils::SigScan::PatternType::RelativePointer, void*)
		AddPattern(LoadScriptedVarInt, "engine_x64_rwdi.dll", "48 89 5C 24 ?? 55 57 41 54 41 56 41 57 48 83 EC ?? 45 0F B6 11 45 33 E4 4D 8B F1 4D 8B F8 48 8B FA 48 8B E9 49 8B D9 45 8B DC 45 84 D2 74 ?? 90 45 0F B6 C2 48 8D 5B ?? 41 80 EA ?? 41 80 FA ?? 44 0F B6 13 41 8D 40 ?? 0F B6 C8 41 0F 47 C8 45 6B DB ?? 0F BE C9 44 03 D9 45 84 D2 75 ?? 0F B6 47 ?? 41 8B D4 84 C0 75 ?? 44 8B 47 ?? EB ?? FE C8 44 0F B6 C0 41 83 E8 ?? 48 89 B4 24 ?? ?? ?? ?? 78 ?? 4C 8B 17 48 B8 ?? ?? ?? ?? ?? ?? ?? ?? 4C 23 D0 41 8D 0C 10 D1 F9 48 63 C1 49 8D 34 C2 49 8B 04 C2 44 39 58 ?? 74 ?? 8D 41 ?? 41 0F 42 C0 44 8B C0 8D 41 ?? 0F 42 D0 41 3B D0 7E ?? B9 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8B F8 48 85 C0 0F 84 ?? ?? ?? ?? 4C 89 60 ?? 4C 89 60 ?? 4C 89 60 ?? 66 44 89 60 ?? 44 88 60 ?? 48 8D 05 ?? ?? ?? ?? 48 89 07 48 8D 05 ?? ?? ?? ?? 48 89 47 ?? E9 ?? ?? ?? ?? 48 85 F6 74 ?? F6 40 ?? ?? 0F 85 ?? ?? ?? ?? B9 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 8B F8 85 C0 0F 84 ?? ?? ?? ?? BA ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 BE ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 1D ?? ?? ?? ?? B9 ?? ?? ?? ?? 48 8B 10", Utils::SigScan::PatternType::Address, void*)
		AddPattern(GetCVarT, "engine_x64_rwdi.dll", "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC ?? 44 0F B6 12 33 F6 41 8B E9", Utils::SigScan::PatternType::Address, void*)
		AddPattern(GetCVarValue, "engine_x64_rwdi.dll", "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 8B D9 41 8B F0 8B FA", Utils::SigScan::PatternType::Address, void*)
		AddPattern(UpdateFrameScript, "engine_x64_rwdi.dll", "48 81 C1 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC 48 81 C1 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC 48 81 C1", Utils::SigScan::PatternType::Address, void*)
		AddPattern(GetListOfCVarValues, "engine_x64_rwdi.dll", "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 8B F9 48 8B 08 8B 49 ?? FF 15 ?? ?? ?? ?? 48 8B CF 48 8B F0 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 8B 09 48 8B 1A FF 15 ?? ?? ?? ?? FF C8 48 8B 54 C3 ?? 80 BA ?? ?? ?? ?? ?? 75 ?? 83 FE ?? 75 ?? 48 8D 47 ?? 48 8B 5C 24 ?? 48 8B 74 24 ?? 48 83 C4 ?? 5F C3 48 8B 5C 24 ?? 48 8D 4F ?? 85 F6 48 8D 47 ?? 48 8B 74 24 ?? 48 0F 44 C1 48 83 C4 ?? 5F C3 0F 57 C0", Utils::SigScan::PatternType::Address, void*)
		AddPattern(HandleTimeWeatherInterpolationOnDemandTextureIsLoaded, "engine_x64_rwdi.dll", "40 84 ED 0F 84 ?? ?? ?? ?? 0F B6 4E", Utils::SigScan::PatternType::Address, void*)

	private:

		static void AddOffsets(DWORD gameVer, const std::unordered_map<std::string, DWORD>& offsets);
		static void AddPatterns(DWORD gameVer, const std::unordered_map<std::string, Utils::SigScan::Pattern>& patterns);

		static std::unordered_map<DWORD, std::unordered_map<std::string, DWORD>>& GetOffsetsMap();
		static std::unordered_map<DWORD, std::unordered_map<std::string, Utils::SigScan::Pattern>>& GetPatternsMap();
	};

#undef AddPattern
#undef AddStaticOffset
#undef AddStaticOffsetFromModule
#undef AddVTOffset
}