#pragma once
#include <EGSDK\Exports.h>

namespace EGSDK {
	struct EGameSDK_API vec4 {
		float X;
		float Y;
		float Z;
		float W;

		vec4();
		vec4(float x, float y, float z, float w);

		bool operator==(const vec4& v) const;
		vec4& operator+=(const vec4& v);
		vec4& operator-=(const vec4& v);
		vec4 operator+(const vec4& v) const;
		vec4 operator-(const vec4& v) const;
		vec4 operator*(const vec4& scalar) const;
		vec4 operator/(const vec4& scalar) const;
		vec4 operator*(float scalar) const;
		vec4 operator/(float scalar) const;

		vec4 operator-() const;

		vec4 normalize() const;
		vec4 round();
		vec4 round(int decimals);

		bool isDefault() const;
	};
}