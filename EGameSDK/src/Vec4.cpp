#define _USE_MATH_DEFINES
#include <cmath>
#include <EGSDK\vec4.h>
#include <EGSDK\Utils\Values.h>

namespace EGSDK {
	vec4::vec4() : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f) {}
	vec4::vec4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

	bool vec4::operator==(const vec4& v) const {
		return Utils::Values::are_samef(X, v.X) && Utils::Values::are_samef(Y, v.Y) && Utils::Values::are_samef(Z, v.Z) && Utils::Values::are_samef(W, v.W);
	}
	vec4& vec4::operator+=(const vec4& v) {
		X += v.X;
		Y += v.Y;
		Z += v.Z;
		W += v.W;
		return *this;
	}
	vec4& vec4::operator-=(const vec4& v) {
		X -= v.X;
		Y -= v.Y;
		Z -= v.Z;
		W -= v.W;
		return *this;
	}
	vec4 vec4::operator+(const vec4& v) const {
		return { X + v.X, Y + v.Y, Z + v.Z, W + v.W };
	}
	vec4 vec4::operator-(const vec4& v) const {
		return { X - v.X, Y - v.Y, Z - v.Z, W - v.W };
	}
	vec4 vec4::operator*(const vec4& scalar) const {
		return { X * scalar.X, Y * scalar.Y, Z * scalar.Z, W * scalar.W };
	}
	vec4 vec4::operator/(const vec4& scalar) const {
		return { X / scalar.X, Y / scalar.Y, Z / scalar.Z, W / scalar.W };
	}
	vec4 vec4::operator*(float scalar) const {
		return { X * scalar, Y * scalar, Z * scalar, W * scalar };
	}
	vec4 vec4::operator/(float scalar) const {
		return { X / scalar, Y / scalar, Z / scalar, W / scalar };
	}

	vec4 vec4::operator-() const {
		return { -X, -Y, -Z, -W };
	}

	vec4 vec4::normalize() const {
		float length = std::sqrt(X * X + Y * Y + Z * Z + W * W);
		return { X / length, Y / length, Z / length, W / length };
	}
	vec4 vec4::round() {
		return { std::roundf(X), std::roundf(Y), std::roundf(Z), std::roundf(W) };
	}
	vec4 vec4::round(int decimals) {
		float power = std::powf(10.0f, static_cast<float>(decimals));
		return { std::roundf(X * power) / power, std::roundf(Y * power) / power, std::roundf(Z * power) / power, std::roundf(W * power) / power };
	}

	bool vec4::isDefault() const {
		return Utils::Values::are_samef(X, 0.0f) && Utils::Values::are_samef(Y, 0.0f) && Utils::Values::are_samef(Z, 0.0f) && Utils::Values::are_samef(W, 0.0f);
	}
}