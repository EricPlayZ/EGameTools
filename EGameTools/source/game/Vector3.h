#pragma once
struct Vector3 {
	float X = 0.0f;
	float Y = 0.0f;
	float Z = 0.0f;

	bool operator==(const Vector3& v) const;
	Vector3& operator+=(const Vector3& v);
	Vector3& operator-=(const Vector3& v);
	Vector3 operator+(const Vector3& v) const;
	Vector3 operator-(const Vector3& v) const;
	Vector3 operator*(float scalar) const;
	Vector3 operator/(float scalar) const;

	Vector3 normalize();
	Vector3 cross(const Vector3& v) const;
	Vector3 round();

	bool isDefault() const;
};