#pragma once
#include <string>

enum class WindowsVersion {
	Unknown,
	Windows7,
	Windows10,
};

namespace Utils {
	extern bool are_same(float a, float b);

	extern bool str_replace(std::string& str, const std::string& from, const std::string& to);

	extern std::string_view GetDesktopDir();
	extern WindowsVersion GetWindowsVersion();
}