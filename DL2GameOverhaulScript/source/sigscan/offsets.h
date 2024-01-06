#pragma once
#include "sigscan.h"
#include "..\memory.h"

#define AddOffset(name, moduleName, pattern, type, retType) \
static retType Get_## name () {\
	static retType name = NULL;\
	if (Memory::IsValidPtr(name)) return name; \
	return name=reinterpret_cast<retType>(PatternScanner::FindPattern(moduleName, {pattern, type}));\
} 
#define AddStaticOffset(name, off) \
static DWORD64 Get_## name () {\
	static DWORD64 name = NULL;\
	if (name) return name; \
	return name=static_cast<DWORD64>(off);\
} 

struct Offsets {
	// ntdll.dll
	AddOffset(LdrpCallInitRoutineOffset, "ntdll.dll", "[48 89 5C 24 08 44 89 44 24 18 48", PatternType::Address, PDWORD64)
	AddOffset(LdrpRunInitializeRoutinesOffset, "ntdll.dll", "[48 89 4C 24 08 53 56 57 41 54 41 55 41 56 41 57 48 81 EC 90", PatternType::Address, PDWORD64) // for win7

	// Input related
	AddOffset(g_CInput, "engine_x64_rwdi.dll", "48 8B 0D [?? ?? ?? ?? 48 85 C9 74 0F 48 8B 01 84 D2", PatternType::RelativePointer, PDWORD64*)

	// Player vars related
	AddStaticOffset(LoadPlayerVariableFuncSize, 0x14E)
	AddOffset(LoadPlayerFloatVariableOffset, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8B D0 48 8D 8C 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 94 24 ?? ?? ?? ?? 48 8B 8C 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 84 24 ?? ?? ?? ??", PatternType::RelativePointer, PDWORD64);
	AddOffset(InitializePlayerVariablesOffset, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8B F0 48 8B CF", PatternType::RelativePointer, DWORD64)
	AddStaticOffset(initPlayerFloatVarsInstrOffset, 0x99);
	AddStaticOffset(initPlayerBoolVarsInstrOffset, 0x2E7);
	AddOffset(PlayerStateOffset, "gamedll_ph_x64_rwdi.dll", "4C 8B 35 [?? ?? ?? ?? 4C 8B E2", PatternType::RelativePointer, LPVOID)

	// Game related
	AddStaticOffset(gameDI_PH2_offset, 0x28)
	AddOffset(CLobbySteamOffset, "engine_x64_rwdi.dll", "48 8B 05 [?? ?? ?? ?? 48 85 C0 74 05 48 83 C0 08 C3", PatternType::RelativePointer, LPVOID)
	AddOffset(g_PlayerObjProperties, "gamedll_ph_x64_rwdi.dll", "48 8B D9 48 89 0D [?? ?? ?? ??", PatternType::RelativePointer, LPVOID)
	AddOffset(g_DayNightCycle, "gamedll_ph_x64_rwdi.dll", "48 8B 0D [?? ?? ?? ?? 48 85 C9 74 10 E8 ?? ?? ?? ?? 84 C0 74 07 B0 01 48 83 C4 28 C3 32 C0", PatternType::RelativePointer, LPVOID)
	AddOffset(g_CameraFPPDI, "gamedll_ph_x64_rwdi.dll", "48 89 05 [?? ?? ?? ?? 40 84 FF", PatternType::RelativePointer, PDWORD64)
	AddOffset(g_FreeCamera, "gamedll_ph_x64_rwdi.dll", "48 89 05 [?? ?? ?? ?? 48 89 4C 24 ??", PatternType::RelativePointer, PDWORD64)

	// Functions
	AddOffset(ReadVideoSettingsOffset, "engine_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8D 4C 24 ?? FF 15 ?? ?? ?? ?? 48 C7 C2 ?? ?? ?? ??", PatternType::RelativePointer, LPVOID)
	AddOffset(CalculateFreeCamCollisionOffset, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8B 06 4C 8D 4C 24 ??", PatternType::RelativePointer, LPVOID)
	AddOffset(GetViewCameraOffset, "engine_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 85 C0 74 28 48 8B C8", PatternType::RelativePointer, LPVOID)
	AddOffset(CreatePlayerHealthModuleOffset, "gamedll_ph_x64_rwdi.dll", "[48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 4C 8B F1 E8 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??", PatternType::Address, LPVOID)
	AddOffset(LifeSetHealth, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 49 8D 4C 24 ?? 49 03 CE", PatternType::RelativePointer, LPVOID)
	AddOffset(TogglePhotoMode, "gamedll_ph_x64_rwdi.dll", "48 83 EC 48 38 91 ?? ?? ?? ??", PatternType::Address, LPVOID)
	AddOffset(GetTimeWeatherSystemOffset, "engine_x64_rwdi.dll", "E8 [?? ?? ?? ?? 33 D2 48 8B C8 E8 ?? ?? ?? ?? 49 8D 4F 38", PatternType::RelativePointer, LPVOID)
	AddOffset(SetForcedWeatherOffset, "engine_x64_rwdi.dll", "89 51 68 C3 CC CC CC CC CC CC CC CC CC CC CC CC", PatternType::Address, LPVOID)
	AddOffset(GetCurrentWeatherOffset, "engine_x64_rwdi.dll", "48 8B 41 78 48 85 C0 75 0F", PatternType::Address, LPVOID)
};

#undef AddOffset
#undef AddStaticOffset