#pragma once
#include <chrono>
#include <iomanip>

template <class result_t = std::chrono::milliseconds, class clock_t = std::chrono::steady_clock, class duration_t = std::chrono::milliseconds> auto since(std::chrono::time_point<clock_t, duration_t> const& start) {
	return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

inline std::ostringstream GetTimestamp() {
	time_t timeInstance = time(0);
	tm timestamp{};
	localtime_s(&timestamp, &timeInstance);
	std::ostringstream oss{};
	oss << "[" << std::setw(2) << std::setfill('0') << timestamp.tm_hour << "h:"
		<< std::setw(2) << std::setfill('0') << timestamp.tm_min << "m:"
		<< std::setw(2) << std::setfill('0') << timestamp.tm_sec << "s] ";

	return oss;
}