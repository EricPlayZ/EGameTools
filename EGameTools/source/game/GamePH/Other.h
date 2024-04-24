#pragma once
#include <string>
#include <basetsd.h>

namespace GamePH {
	extern const DWORD GetCurrentGameVersion();
	extern const std::string GameVerToStr(DWORD version);
	extern const std::string GetCurrentGameVersionStr();
	extern void ShowTPPModel(bool showTPPModel);
	extern bool ReloadJumps();
}
