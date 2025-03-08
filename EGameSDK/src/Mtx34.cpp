#include <EGSDK\mtx34.h>

mtx34::mtx34() : Row1(1.0f, 0.0f, 0.0f, 0.0f), Row2(0.0f, 1.0f, 0.0f, 0.0f), Row3(0.0f, 0.0f, 1.0f, 0.0f) {}
mtx34::mtx34(const vec4& row1, const vec4& row2, const vec4& row3) : Row1(row1), Row2(row2), Row3(row3) {}

bool mtx34::operator==(const mtx34& m) const {
	return Row1 == m.Row1 && Row2 == m.Row2 && Row3 == m.Row3;
}
mtx34& mtx34::operator+=(const mtx34& m) {
	Row1 += m.Row1;
	Row2 += m.Row2;
	Row3 += m.Row3;
	return *this;
}
mtx34& mtx34::operator-=(const mtx34& m) {
	Row1 -= m.Row1;
	Row2 -= m.Row2;
	Row3 -= m.Row3;
	return *this;
}
mtx34 mtx34::operator+(const mtx34& m) const {
	return { Row1 + m.Row1, Row2 + m.Row2, Row3 + m.Row3 };
}
mtx34 mtx34::operator-(const mtx34& m) const {
	return { Row1 - m.Row1, Row2 - m.Row2, Row3 - m.Row3 };
}
mtx34 mtx34::operator*(const mtx34& scalar) const {
	return { Row1 * scalar.Row1, Row2 * scalar.Row2, Row3 * scalar.Row3 };
}
mtx34 mtx34::operator/(const mtx34& scalar) const {
	return { Row1 / scalar.Row1, Row2 / scalar.Row2, Row3 / scalar.Row3 };
}
mtx34 mtx34::operator*(const vec4& scalar) const {
	return { Row1 * scalar, Row2 * scalar, Row3 * scalar };
}
mtx34 mtx34::operator/(const vec4& scalar) const {
	return { Row1 / scalar, Row2 / scalar, Row3 / scalar };
}

vec3 mtx34::GetXAxis() const {
	return vec3(Row1.X, Row2.X, Row3.X);
}
vec3 mtx34::GetYAxis() const {
	return vec3(Row1.Y, Row2.Y, Row3.Y);
}
vec3 mtx34::GetZAxis() const {
	return vec3(Row1.Z, Row2.Z, Row3.Z);
}
vec3 mtx34::GetPosition() const {
	return vec3(Row1.W, Row2.W, Row3.W);
}

mtx34 mtx34::normalize() {
	return { Row1.normalize(), Row2.normalize(), Row3.normalize() };
}