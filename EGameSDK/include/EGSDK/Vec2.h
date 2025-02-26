#pragma once
#include <EGSDK\Exports.h>

namespace EGSDK {
	struct EGameSDK_API vec2 {
		float X;
		float Y;

		vec2();
		vec2(float x, float y);

		bool operator==(const vec2& v) const;
		vec2& operator+=(const vec2& v);
		vec2& operator-=(const vec2& v);
		vec2 operator+(const vec2& v) const;
		vec2 operator-(const vec2& v) const;
		vec2 operator*(const vec2& scalar) const;
		vec2 operator/(const vec2& scalar) const;
		vec2 operator*(float scalar) const;
		vec2 operator/(float scalar) const;

		vec2 operator-() const;

		vec2 normalize() const;
		float dot(const vec2& v) const;
		vec2 round();
		vec2 round(int decimals);

		bool isDefault() const;
	};
}