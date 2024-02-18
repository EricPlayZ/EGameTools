#include <pch.h>

namespace Utils {
	namespace Time {
        Timer::Timer(long timeMs) : timeToPass(std::chrono::milliseconds(timeMs)), timePassed(false) {
            start = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now());
        }

        const long long Timer::GetTimePassed() {
            if (timePassed)
                return -1;

            const auto end = clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            const long long timePassedMs = duration.count();

            if (timePassedMs < 0) {
                start = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now());
                return -1;
            }
            return timePassedMs;
        }
        const bool Timer::DidTimePass() {
            if (timePassed)
                return timePassed;

            const auto end = clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            const long long timePassedMs = duration.count();

            if (timePassedMs < 0) {
                start = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now());
                return false;
            }

            if (duration >= timeToPass)
                timePassed = true;
            return timePassed;
        }

		std::ostringstream GetTimestamp() {
			time_t timeInstance = time(0);
			tm timestamp{};
			localtime_s(&timestamp, &timeInstance);

			std::ostringstream oss{};
			oss << "[" << std::setw(2) << std::setfill('0') << timestamp.tm_hour << "h:"
				<< std::setw(2) << std::setfill('0') << timestamp.tm_min << "m:"
				<< std::setw(2) << std::setfill('0') << timestamp.tm_sec << "s] ";

			return oss;
		}
	}
}