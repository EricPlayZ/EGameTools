#pragma once
#include <EGSDK\vec3.h>
#include <EGSDK\vec4.h>
#include <EGSDK\Exports.h>

struct EGameSDK_API alignas(16) mtx34 {
	vec4 Row1;
	vec4 Row2;
	vec4 Row3;

    mtx34();
    mtx34(const vec4& row1, const vec4& row2, const vec4& row3);

	bool operator==(const mtx34& m) const;
	mtx34& operator+=(const mtx34& m);
	mtx34& operator-=(const mtx34& m);
	mtx34 operator+(const mtx34& m) const;
	mtx34 operator-(const mtx34& m) const;
	mtx34 operator*(const mtx34& scalar) const;
	mtx34 operator/(const mtx34& scalar) const;
	mtx34 operator*(const vec4& scalar) const;
	mtx34 operator/(const vec4& scalar) const;

	vec3 GetXAxis() const;
	vec3 GetYAxis() const;
	vec3 GetZAxis() const;
	vec3 GetPosition() const;

	mtx34 normalize();
};