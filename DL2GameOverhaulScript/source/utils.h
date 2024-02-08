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
		std::chrono::milliseconds timeToPass;
		Timer() : timeToPass(0), timePassed(false) {}
		Timer(long timeMs);

		long GetTimePassed();
		bool DidTimePass();
	private:
		time_point_type start;
		bool timePassed;
	};

	extern bool are_same(float a, float b, float precision = 0.0001f);
	extern float round_dec(float value, int decimal_places = 2);

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
	extern std::filesystem::path GetCurrentProcDirectoryFS();
	extern std::string GetCurrentProcDirectory();
	extern bool FileExistsInDir(const char* fileName, const char* dir);
	extern WindowsVersion GetWindowsVersion();
}