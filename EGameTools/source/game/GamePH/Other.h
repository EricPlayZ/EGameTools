#pragma once
#include <string>
#include <basetsd.h>

namespace GamePH {
	extern const DWORD64 GetCurrentGameVersion();
	extern const std::string GameVerToStr(DWORD64 version);
	extern const std::string GetCurrentGameVersionStr();
	extern void ShowTPPModel(bool showTPPModel);
	extern bool ReloadJumps();
}