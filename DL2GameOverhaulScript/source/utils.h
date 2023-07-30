#pragma once
enum class WindowsVersion {
	Unknown,
	Windows7,
	Windows10,
};

namespace Utils {
	extern WindowsVersion GetWindowsVersion();
}