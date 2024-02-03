#pragma once
#include <chrono>
#include <string>
#include <windows.h>

enum class WindowsVersion {
	Unknown,
	Windows7,
	Windows10,
};

namespace Utils {
	class Timer {
		using clock = std::chrono::system_clock;
		using time_point_type = std::chrono::time_point<clock, std::chrono::milliseconds>;
	public:
		long timeToPass;
		Timer(long timeMs);

		bool GetTimePassed();
	private:
		time_point_type start;
		bool timePassed;
	};

	extern bool are_same(float a, float b);

	extern bool str_replace(std::string& str, const std::string& from, const std::string& to);
	template <typename T>
	auto to_string(T val) {
		if constexpr (std::is_same<T, std::string>::value)
			return static_cast<std::string>(val);
		else
			return std::to_string(val);
	}

	extern FARPROC GetProcAddr(std::string_view module, std::string_view funcName);

	extern std::string_view GetDesktopDir();
	extern std::string_view GetDocumentsDir();
	extern WindowsVersion GetWindowsVersion();
}