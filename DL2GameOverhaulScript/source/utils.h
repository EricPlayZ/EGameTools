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
	template <typename T>
	auto to_string(T val) {
		if constexpr (std::is_same<T, std::string>::value)
			return static_cast<std::string>(val);
		else
			return std::to_string(val);
	}

	extern std::string_view GetDesktopDir();
	extern WindowsVersion GetWindowsVersion();
}