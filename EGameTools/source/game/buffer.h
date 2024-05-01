#pragma once
#pragma pack(1)

template<size_t size, typename T> class buffer {
	char buffer[size];
public:
	T data;

	operator T() { return data; }
	T operator->() { return data; }

	DWORD64 operator&(const DWORD64 other) const { return reinterpret_cast<DWORD64>(data) & other; }
	DWORD64 operator>>(const int shift) const { return reinterpret_cast<DWORD64>(data) >> shift; }
	DWORD64 operator<<(const int shift) const { return reinterpret_cast<DWORD64>(data) << shift; }

	T& operator=(const T& other) { data = other; return data; }
	T& operator*=(const T& other) { data *= other; return data; }
	T operator*(const T& other) const { return data * other; }
	T& operator/=(const T& other) { data /= other; return data; }
	T operator/(const T& other) const { return data / other; }
	T& operator+=(const T& other) { data += other; return data; }
	T operator+(const T& other) const { return data + other; }
	T& operator-=(const T& other) { data -= other; return data; }
	T operator-(const T& other) const { return data - other; }
};

#pragma pack()