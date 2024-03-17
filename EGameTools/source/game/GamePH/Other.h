#pragma once
#include <string>
#include <basetsd.h>

namespace GamePH {
	extern const std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> GetCurrentGameVersion();
	extern const std::string GetCurrentGameVersionStr();
	extern const std::string GameVerToStr(uint16_t version);
	extern void ShowTPPModel(bool showTPPModel);
	extern bool ReloadJumps();
}