#pragma once
#include "..\memory.h"
#include "sigscan.h"

#define AddOffset(name, moduleName, pattern, type, retType)\
static retType Get_## name () {\
	static retType name = NULL;\
	if (Memory::IsValidPtr(name)) return name;\
	return name=reinterpret_cast<retType>(PatternScanner::FindPattern(moduleName, {pattern, type}));\
} 
#define AddStaticOffset(name, off)\
static DWORD64 Get_## name () {\
	static DWORD64 name = 0;\
	if (name) return name; \
	return name=static_cast<DWORD64>(off);\
} 
#define AddStaticOffset2(name, moduleName, off) \
static DWORD64 Get_## name () {\
	static DWORD64 name = 0;\
	if (Memory::IsValidPtr(name)) return name;\
	return name=reinterpret_cast<DWORD64>(GetModuleHandle(moduleName)) + static_cast<DWORD64>(off);\
} 

struct Offsets {
	// ntdll.dll
	//AddOffset(LdrpCallInitRoutine, "ntdll.dll", "[48 89 5C 24 08 44 89 44 24 18 48", PatternType::Address, PDWORD64)
	//AddOffset(LdrpRunInitializeRoutines, "ntdll.dll", "[48 89 4C 24 08 53 56 57 41 54 41 55 41 56 41 57 48 81 EC 90", PatternType::Address, PDWORD64) // for win7

	// Input related
	AddOffset(g_CInput, "engine_x64_rwdi.dll", "48 8B 0D [?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 01 84 D2", PatternType::RelativePointer, PDWORD64*)
	//AddOffset(WndProc, "engine_x64_rwdi.dll", "40 55 56 57 41 54 41 56 48 83 EC ?? 49 8B E9", PatternType::Address, LPVOID)

	// Player vars related
	AddStaticOffset(LoadPlayerVariableFunc_size, 0x14E)
	//AddOffset(LoadPlayerFloatVariable, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8B D0 48 8D 8C 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 94 24 ?? ?? ?? ?? 48 8B 8C 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 84 24 ?? ?? ?? ??", PatternType::RelativePointer, PDWORD64);
	AddOffset(InitializePlayerVariables, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8B F0 48 8B CF", PatternType::RelativePointer, DWORD64)
	AddStaticOffset(initPlayerFloatVarsInstr_offset, 0x99);
	AddStaticOffset(initPlayerBoolVarsInstr_offset, 0x2E7);
	AddOffset(PlayerState, "gamedll_ph_x64_rwdi.dll", "4C 8B 35 [?? ?? ?? ?? 4C 8B E2", PatternType::RelativePointer, LPVOID)

	// Game related
	AddStaticOffset(gameDI_PH2_offset, 0x28)
	AddOffset(CLobbySteam, "engine_x64_rwdi.dll", "48 8B 05 [?? ?? ?? ?? 48 85 C0 74 ?? 48 83 C0", PatternType::RelativePointer, LPVOID)
	AddOffset(g_PlayerObjProperties, "gamedll_ph_x64_rwdi.dll", "48 89 0D [?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0", PatternType::RelativePointer, LPVOID)
	AddOffset(g_DayNightCycle, "gamedll_ph_x64_rwdi.dll", "48 8B 0D [?? ?? ?? ?? 48 85 C9 74 ?? E8 ?? ?? ?? ?? 84 C0 74 ?? B0 ?? 48 83 C4 ?? C3 32 C0", PatternType::RelativePointer, LPVOID)
	//AddOffset(g_CameraFPPDI, "gamedll_ph_x64_rwdi.dll", "48 89 05 [?? ?? ?? ?? 40 84 FF", PatternType::RelativePointer, PDWORD64)
	AddOffset(g_FreeCamera, "gamedll_ph_x64_rwdi.dll", "48 89 05 [?? ?? ?? ?? 48 89 4C 24", PatternType::RelativePointer, PDWORD64)
	AddStaticOffset2(g_BackgroundModuleScreenController, "gamedll_ph_x64_rwdi.dll", 0x3377760)
	//AddOffset(CameraFPPDI_VT, "gamedll_ph_x64_rwdi.dll", "48 8D 05 [?? ?? ?? ?? 48 89 07 48 8D 4F 60", PatternType::RelativePointer, DWORD64)

	// Functions
	AddOffset(ReadVideoSettings, "engine_x64_rwdi.dll", "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B FA 48 8B D9 45 84 C0", PatternType::Address, LPVOID)
	AddOffset(GetCurrentGameVersion, "gamedll_ph_x64_rwdi.dll", "B8 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC CC 48 83 79", PatternType::Address, LPVOID)
	AddOffset(CalculateFreeCamCollision, "gamedll_ph_x64_rwdi.dll", "48 8B C4 55 53 56 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 B9", PatternType::Address, LPVOID)
	//AddOffset(GetViewCamera, "engine_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 85 C0 74 28 48 8B C8", PatternType::RelativePointer, LPVOID)
	AddOffset(CreatePlayerHealthModule, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 4C 8B F1 E8 ?? ?? ?? ?? 48 8D 05", PatternType::Address, LPVOID)
	AddOffset(LifeSetHealth, "gamedll_ph_x64_rwdi.dll", "F3 0F 11 49 ?? F3 0F 11 49 ?? F3 0F 11 49 ?? C3", PatternType::Address, LPVOID)
	AddOffset(TogglePhotoMode, "gamedll_ph_x64_rwdi.dll", "48 83 EC ?? 38 91 ?? ?? ?? ?? 0F 84", PatternType::Address, LPVOID)
	//AddOffset(OnUpdate_ChangeMap, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 88 44 24 20 48 8B 84 24 ?? ?? ?? ?? 48 83 78 ?? ??", PatternType::RelativePointer, LPVOID)
	AddOffset(ShowTPPModelFunc2, "gamedll_ph_x64_rwdi.dll", "40 53 48 83 EC ?? 48 8B 01 48 8B D9 FF 90 ?? ?? ?? ?? 84 C0 74 ?? 48 8B 83 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 48 85 C9 74 ?? 48 83 B9 ?? ?? ?? ?? ?? 74 ?? 48 8B 81", PatternType::Address, LPVOID)
	AddOffset(ShowTPPModelFunc3, "gamedll_ph_x64_rwdi.dll", "48 83 EC ?? 38 91 ?? ?? ?? ?? 74 ?? 88 91", PatternType::Address, LPVOID)
	//AddOffset(CalculateOutOfBoundsTimer, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 0F B6 99", PatternType::Address, LPVOID)
	AddOffset(IsNotOutOfBounds, "gamedll_ph_x64_rwdi.dll", "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 4C 8B F9 48 85 D2", PatternType::Address, LPVOID)
	AddOffset(ReloadJumps, "gamedll_ph_x64_rwdi.dll", "48 83 EC ?? E8 ?? ?? ?? ?? 48 8D 15", PatternType::Address, LPVOID)
	AddOffset(PlaySoundEvent, "gamedll_ph_x64_rwdi.dll", "4C 8B DC 49 89 5B ?? 49 89 73 ?? 57 48 81 EC ?? ?? ?? ?? 4C 8B 4C 24 ?? 48 8B F9 4D 8B D0 66 C7 84 24 ?? ?? ?? ?? ?? ?? 49 8B C1 66 C7 84 24", PatternType::Address, LPVOID)
	//AddOffset(GetTimeWeatherSystem, "engine_x64_rwdi.dll", "E8 [?? ?? ?? ?? 33 D2 48 8B C8 E8 ?? ?? ?? ?? 49 8D 4F 38", PatternType::RelativePointer, LPVOID)
	//AddOffset(SetForcedWeather, "engine_x64_rwdi.dll", "89 51 68 C3 CC CC CC CC CC CC CC CC CC CC CC CC", PatternType::Address, LPVOID)
	//AddOffset(GetCurrentWeather, "engine_x64_rwdi.dll", "48 8B 41 78 48 85 C0 75 0F", PatternType::Address, LPVOID)
	AddOffset(MoveCameraFromForwardUpPos, "engine_x64_rwdi.dll", "48 89 5C 24 ?? 57 48 83 EC ?? 49 8B C1 48 8B F9", PatternType::Address, LPVOID)
	//AddOffset(GetForwardVector, "engine_x64_rwdi.dll", "4C 8B 41 38 41 8B 40 48", PatternType::Address, LPVOID)
	//AddOffset(IsLoading, "engine_x64_rwdi.dll", "48 8B 05 ?? ?? ?? ?? 48 8B 51 38", PatternType::Address, LPVOID)
	//AddOffset(ShowUIManager, "engine_x64_rwdi.dll", "48 8B 0D ?? ?? ?? ?? E9 ?? ?? ?? ?? CC CC CC CC 48 8B 49 ?? E9 ?? ?? ?? ?? CC CC CC CC CC CC CC 48 8B 49 ?? E9 ?? ?? ?? ?? CC CC CC CC CC CC CC 48 8B 49 ?? E9 ?? ?? ?? ?? CC CC CC CC CC CC CC 48 8B 49 ?? E9 ?? ?? ?? ?? CC CC CC CC CC CC CC 40 53", PatternType::Address, LPVOID)
	//AddOffset(GetGameTimeDelta, "engine_x64_rwdi.dll", "E8 [?? ?? ?? ?? F3 0F 59 05 ?? ?? ?? ?? F3 0F 58 03", PatternType::RelativePointer, LPVOID)
};

#undef AddOffset
#undef AddStaticOffset
#undef AddStaticOffset2