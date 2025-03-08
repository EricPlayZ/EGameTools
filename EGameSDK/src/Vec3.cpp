#define _USE_MATH_DEFINES
#include <cmath>
#include <EGSDK\vec3.h>
#include <EGSDK\Utils\Values.h>

vec3::vec3() : X(0.0f), Y(0.0f), Z(0.0f) {}
vec3::vec3(float x, float y, float z) : X(x), Y(y), Z(z) {}

bool vec3::operator==(const vec3& v) const {
	return EGSDK::Utils::Values::are_samef(X, v.X) && EGSDK::Utils::Values::are_samef(Y, v.Y) && EGSDK::Utils::Values::are_samef(Z, v.Z);
}
vec3& vec3::operator+=(const vec3& v) {
	X += v.X;
	Y += v.Y;
	Z += v.Z;
	return *this;
}
vec3& vec3::operator-=(const vec3& v) {
	X -= v.X;
	Y -= v.Y;
	Z -= v.Z;
	return *this;
}
vec3 vec3::operator+(const vec3& v) const {
	return { X + v.X, Y + v.Y, Z + v.Z };
}
vec3 vec3::operator-(const vec3& v) const {
	return { X - v.X, Y - v.Y, Z - v.Z };
}
vec3 vec3::operator*(const vec3& scalar) const {
	return { X * scalar.X, Y * scalar.Y, Z * scalar.Z };
}
vec3 vec3::operator/(const vec3& scalar) const {
	return { X / scalar.X, Y / scalar.Y, Z / scalar.Z };
}
vec3 vec3::operator*(float scalar) const {
	return { X * scalar, Y * scalar, Z * scalar };
}
vec3 vec3::operator/(float scalar) const {
	return { X / scalar, Y / scalar, Z / scalar };
}

vec3 vec3::operator-() const {
	return { -X, -Y, -Z };
}

vec3 vec3::normalize() const {
	if (isDefault())
		return *this;

	float length = std::sqrt(X * X + Y * Y + Z * Z);
	return { X / length, Y / length, Z / length };
}
vec3 vec3::cross(const vec3& v) const {
	return {
		Y * v.Z - Z * v.Y,
		Z * v.X - X * v.Z,
		X * v.Y - Y * v.X
	};
}
float vec3::dot(const vec3& v) const {
	return (X * v.X) + (Y * v.Y) + (Z * v.Z);
}
vec3 vec3::round() {
	return { std::roundf(X), std::roundf(Y), std::roundf(Z) };
}
vec3 vec3::round(int decimals) {
	float power = std::powf(10.0f, static_cast<float>(decimals));
	return { std::roundf(X * power) / power, std::roundf(Y * power) / power, std::roundf(Z * power) / power };
}

bool vec3::isDefault() const {
	return EGSDK::Utils::Values::are_samef(X, 0.0f) && EGSDK::Utils::Values::are_samef(Y, 0.0f) && EGSDK::Utils::Values::are_samef(Z, 0.0f);
}