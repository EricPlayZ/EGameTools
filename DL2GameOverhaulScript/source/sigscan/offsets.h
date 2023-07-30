#pragma once
#include "sigscan.h"
#include "../memory.h"

// macro for lazily adding signatures to this sig list
#define AddOffset(name, moduleName, pattern, type, retType) \
static retType Get_## name () {\
	static retType name = NULL;\
	if (Memory::IsValidPtr(name)) return name; \
	return name=reinterpret_cast<retType>(PatternScanner::FindPattern(moduleName, {pattern, type}));\
} 

// macro for lazily adding offsets to the sig list
#define AddStaticOffset(name, off) \
static DWORD64 Get_## name () {\
	static DWORD64 name = NULL;\
	if (name != NULL) return name; \
	return name=static_cast<DWORD64>(off);\
} 

// Static offsets go here
struct Offsets {
	// ntdll.dll
	AddOffset(LdrpCallInitRoutineOffset, "ntdll.dll", "[48 89 5C 24 08 44 89 44 24 18 48", PatternType::Address, DWORD64)
	AddOffset(LdrpRunInitializeRoutinesOffset, "ntdll.dll", "[48 89 4C 24 08 53 56 57 41 54 41 55 41 56 41 57 48 81 EC 90", PatternType::Address, DWORD64) // for win7

	// Input related
	AddOffset(g_gui__CActionNodeOffset, "engine_x64_rwdi.dll", "C3 48 8B 0D [?? ?? ?? ?? E8 ?? ?? ?? ?? 33 C0", PatternType::RelativePointer, LPVOID*)
	AddOffset(DoGameWindowStuffOnLostFocusOffset, "engine_x64_rwdi.dll", "C3 48 8B 0D ?? ?? ?? ?? E8 [?? ?? ?? ?? 33 C0", PatternType::RelativePointer, DWORD64)
	AddOffset(DoGameWindowStuffOnGainFocusOffset, "engine_x64_rwdi.dll", "48 8B 0D ?? ?? ?? ?? E8 [?? ?? ?? ?? BA ?? ?? ?? ?? 48 8B CE E8 ?? ?? ?? ??", PatternType::RelativePointer, DWORD64)

	// Player vars related
	AddOffset(LoadPlayerFloatVariableOffset, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8D 55 80 48 8B 08", PatternType::RelativePointer, DWORD64);
	AddOffset(LoadPlayerBoolVariableOffset, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8D 55 98 48 8B 08", PatternType::RelativePointer, DWORD64);
	AddOffset(InitializePlayerVariablesOffset, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8B E8 48 8B CF 48 89 AF ?? ?? ?? ??", PatternType::RelativePointer, DWORD64)
	AddStaticOffset(initPlayerFloatVarsInstrOffset, 0xE);
	AddStaticOffset(initPlayerBoolVarsInstrOffset, 0x234);
	AddOffset(PlayerStateOffset, "gamedll_ph_x64_rwdi.dll", "48 8B 3D [?? ?? ?? ?? 4C 8B EA", PatternType::RelativePointer, DWORD64)

	// Game related
	AddOffset(CLobbySteamOffset, "engine_x64_rwdi.dll", "48 8B 05 [?? ?? ?? ?? 48 85 C0 74 05 48 83 C0 08 C3", PatternType::RelativePointer, DWORD64)
	AddStaticOffset(gameDI_PH2Offset, 0x18)
	AddOffset(g_PlayerObjProperties, "gamedll_ph_x64_rwdi.dll", "48 8B D9 48 89 0D [?? ?? ?? ??", PatternType::RelativePointer, DWORD64)

	// Functions
	AddOffset(CalculateFreeCamCollisionOffset, "gamedll_ph_x64_rwdi.dll", "E8 [?? ?? ?? ?? 48 8B 06 4C 8D 4C 24 ??", PatternType::RelativePointer, LPVOID)
	AddOffset(MoveCharacterOffset, "engine_x64_rwdi.dll", "E8 [?? ?? ?? ?? 80 BB ?? ?? ?? ?? ?? 74 4E", PatternType::RelativePointer, DWORD64)
};

#undef AddFuncCall
#undef AddOffset
#undef AddStaticOffset