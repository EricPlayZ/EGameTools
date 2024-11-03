#pragma once
#include <chrono>
#include <iomanip>

namespace Utils {
	namespace Time {
		class Timer {
			using clock = std::chrono::steady_clock;
			using time_point_type = std::chrono::time_point<clock, std::chrono::milliseconds>;
		public:
			std::chrono::milliseconds timeToPass;
			Timer() : timeToPass(0), timePassed(false) {}
			Timer(long timeMs);

			const long long GetTimePassed();
			const bool DidTimePass();
		private:
			time_point_type start;
			time_point_type end;
			bool timePassed;
		};

		template <class result_t = std::chrono::milliseconds, class clock_t = std::chrono::steady_clock, class duration_t = std::chrono::milliseconds> auto since(std::chrono::time_point<clock_t, duration_t> const& start) {
			return std::chrono::duration_cast<result_t>(clock_t::now() - start);
		}

		extern std::ostringstream GetTimestamp();
	}
}