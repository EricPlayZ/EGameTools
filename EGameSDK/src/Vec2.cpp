#define _USE_MATH_DEFINES
#include <cmath>
#include <EGSDK\vec2.h>
#include <EGSDK\Utils\Values.h>

vec2::vec2() : X(0.0f), Y(0.0f) {}
vec2::vec2(float x, float y) : X(x), Y(y) {}

bool vec2::operator==(const vec2& v) const {
	return EGSDK::Utils::Values::are_samef(X, v.X) && EGSDK::Utils::Values::are_samef(Y, v.Y);
}
vec2& vec2::operator+=(const vec2& v) {
	X += v.X;
	Y += v.Y;
	return *this;
}
vec2& vec2::operator-=(const vec2& v) {
	X -= v.X;
	Y -= v.Y;
	return *this;
}
vec2 vec2::operator+(const vec2& v) const {
	return { X + v.X, Y + v.Y };
}
vec2 vec2::operator-(const vec2& v) const {
	return { X - v.X, Y - v.Y };
}
vec2 vec2::operator*(const vec2& scalar) const {
	return { X * scalar.X, Y * scalar.Y };
}
vec2 vec2::operator/(const vec2& scalar) const {
	return { X / scalar.X, Y / scalar.Y };
}
vec2 vec2::operator*(float scalar) const {
	return { X * scalar, Y * scalar };
}
vec2 vec2::operator/(float scalar) const {
	return { X / scalar, Y / scalar };
}

vec2 vec2::operator-() const {
	return { -X, -Y };
}

vec2 vec2::normalize() const {
	if (isDefault())
		return *this;

	float length = std::sqrt(X * X + Y * Y);
	return { X / length, Y / length };
}
float vec2::dot(const vec2& v) const {
	return (X * v.X) + (Y * v.Y);
}
vec2 vec2::round() {
	return { std::roundf(X), std::roundf(Y) };
}
vec2 vec2::round(int decimals) {
	float power = std::powf(10.0f, static_cast<float>(decimals));
	return { std::roundf(X * power) / power, std::roundf(Y * power) / power };
}

bool vec2::isDefault() const {
	return EGSDK::Utils::Values::are_samef(X, 0.0f) && EGSDK::Utils::Values::are_samef(Y, 0.0f);
}