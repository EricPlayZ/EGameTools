#pragma once
#include <EGSDK\Exports.h>

struct EGameSDK_API vec3 {
	float X;
	float Y;
	float Z;

	vec3();
	vec3(float x, float y, float z);

	bool operator==(const vec3& v) const;
	vec3& operator+=(const vec3& v);
	vec3& operator-=(const vec3& v);
	vec3 operator+(const vec3& v) const;
	vec3 operator-(const vec3& v) const;
	vec3 operator*(const vec3& scalar) const;
	vec3 operator/(const vec3& scalar) const;
	vec3 operator*(float scalar) const;
	vec3 operator/(float scalar) const;

	vec3 operator-() const;

	vec3 normalize() const;
	vec3 cross(const vec3& v) const;
	float dot(const vec3& v) const;
	vec3 round();
	vec3 round(int decimals);

	bool isDefault() const;
};