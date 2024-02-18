#pragma once
struct Vector3 {
	float X, Y, Z;

	bool operator==(const Vector3& v) const;
	Vector3& operator+=(const Vector3& v);
	Vector3& operator-=(const Vector3& v);
	Vector3 operator+(const Vector3& v) const;
	Vector3 operator-(const Vector3& v) const;
	Vector3 operator*(float scalar) const;
	Vector3 operator/(float scalar) const;

	Vector3 normalize();
	Vector3 cross(const Vector3& v) const;

	bool isDefault() const;
};