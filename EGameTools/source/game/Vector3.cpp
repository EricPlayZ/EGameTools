#include <pch.h>

bool Vector3::operator==(const Vector3& v) const {
	return Utils::Values::are_samef(X, v.X) && Utils::Values::are_samef(Y, v.Y) && Utils::Values::are_samef(Z, v.Z);
}
Vector3& Vector3::operator+=(const Vector3& v) {
	X += v.X;
	Y += v.Y;
	Z += v.Z;
	return *this;
}
Vector3& Vector3::operator-=(const Vector3& v) {
	X -= v.X;
	Y -= v.Y;
	Z -= v.Z;
	return *this;
}
Vector3 Vector3::operator+(const Vector3& v) const {
	return { X + v.X, Y + v.Y, Z + v.Z };
}
Vector3 Vector3::operator-(const Vector3& v) const {
	return { X - v.X, Y - v.Y, Z - v.Z };
}
Vector3 Vector3::operator*(float scalar) const {
	return { X * scalar, Y * scalar, Z * scalar };
}
Vector3 Vector3::operator/(float scalar) const {
	return { X / scalar, Y / scalar, Z / scalar };
}

Vector3 Vector3::normalize() {
	float length = std::sqrt(X * X + Y * Y + Z * Z);
	return { X / length, Y / length, Z / length };
}
Vector3 Vector3::cross(const Vector3& v) const {
	return {
		Y * v.Z - Z * v.Y,
		Z * v.X - X * v.Z,
		X * v.Y - Y * v.X
	};
}

bool Vector3::isDefault() const {
	return Utils::Values::are_samef(X, 0.0f) && Utils::Values::are_samef(Y, 0.0f) && Utils::Values::are_samef(Z, 0.0f);
}