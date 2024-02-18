#pragma once
#include <Windows.h>
#include <spdlog\logger.h>

namespace Utils {
	enum ConsoleColors {
		c_black,
		c_blue,
		c_green,
		c_aqua,
		c_red,
		c_purple,
		c_yellow,
		c_white,
		c_gray,
		c_lightblue,
		c_lightgreen,
		c_lightaqua,
		c_lightred,
		c_lightpurple,
		c_lightyellow,
		c_brightwhite
	};

	template<typename... Args> void PrintError(const std::string f, Args... args) {
		spdlog::error(fmt::runtime(f.c_str()), std::forward<Args>(args)...);
	}
	template<typename... Args> void PrintWarning(const std::string f, Args... args) {
		spdlog::warn(fmt::runtime(f.c_str()), std::forward<Args>(args)...);
	}
	template<typename... Args> void PrintInfo(const std::string f, Args... args) {
		spdlog::info(fmt::runtime(f.c_str()), std::forward<Args>(args)...);
	}
}