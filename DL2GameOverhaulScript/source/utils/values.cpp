#include <pch.h>

namespace Utils {
    namespace Values {
        const bool are_samef(float a, float b, float precision) { return abs(a - b) < precision; }
        float round_decimal(float value, int decimal_places) {
            const double multiplier = std::pow(10.0f, decimal_places);
            return std::roundf(value * static_cast<float>(multiplier)) / static_cast<float>(multiplier);
        }

        const bool str_replace(std::string& str, const std::string& from, const std::string& to) {
            const size_t start_pos = str.find(from);
            if (start_pos == std::string::npos)
                return false;

            str.replace(start_pos, from.length(), to);
            return true;
        }
    }
}