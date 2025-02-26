#include <string>
#include <algorithm>
#include <array>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <EGSDK\Utils\Values.h>

namespace EGSDK::Utils {
    namespace Values {
        bool are_samef(float a, float b, float precision) {
            return std::fabs(a - b) < precision;
        }
        float round_decimal(float value, int decimal_places) {
            float multiplier = 1.0f;
            for (int i = 0; i < decimal_places; ++i)
                multiplier *= 10.0f;

            return std::round(value * multiplier) / multiplier;
        }

        float GetPitchDegreesRelativeTo(const vec3& dirVec, const vec3& referenceAxis) {
            vec3 relativeDirVec = dirVec;

            if (referenceAxis == vec3(0.0f, 1.0f, 0.0f))
                relativeDirVec = vec3(dirVec.X, dirVec.Z, dirVec.Y);
            else if (referenceAxis == vec3(1.0f, 0.0f, 0.0f))
                relativeDirVec = vec3(dirVec.Z, dirVec.Y, dirVec.X);

            vec3 normalizedVec = relativeDirVec.normalize();
            float pitchRadians = std::atan2(normalizedVec.Y, std::sqrt(normalizedVec.X * normalizedVec.X + normalizedVec.Z * normalizedVec.Z));
            return pitchRadians * (180.0f / M_PI);
        }

        bool str_ends_with_ci(const std::string& text, const std::string& substr) {
            if (substr.length() > text.length())
                return false;

            return std::equal(substr.rbegin(), substr.rend(), text.rbegin(), [](char ch1, char ch2) {
                return std::toupper(ch1) == std::toupper(ch2);
            });
        }
        bool str_replace(std::string& str, const std::string& from, const std::string& to) {
            size_t start_pos = str.find(from);
            if (start_pos == std::string::npos)
                return false;

            str.replace(start_pos, from.length(), to);
            return true;
        }
        std::string to_lower(const std::string& str) {
            std::string lowerStr = str;
            std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), tolower);
            return lowerStr;
        }

        std::string GetSimpleRTTITypeName(std::string fullName) {
            static const std::string class_prefix = "class ";
            static const std::string struct_prefix = "struct ";
            static const std::string enum_prefix = "enum ";

            if (fullName.compare(0, class_prefix.size(), class_prefix) == 0)
                fullName.erase(0, class_prefix.size());
            else if (fullName.compare(0, struct_prefix.size(), struct_prefix) == 0)
                fullName.erase(0, struct_prefix.size());
            else if (fullName.compare(0, enum_prefix.size(), enum_prefix) == 0)
                fullName.erase(0, enum_prefix.size());

            size_t pos = fullName.find_last_of("::");
            if (pos != std::string::npos)
                fullName.erase(0, pos + 1);

            return fullName;
        }
    }
}