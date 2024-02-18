#pragma once
namespace Utils {
	namespace Windows {
		enum class WindowsVersion {
			Unknown,
			Windows7,
			Windows10,
		};

		extern const WindowsVersion GetWindowsVersion();
	}
}