#pragma once
#pragma pack(1)

template<size_t size, typename T> class buffer {
	char buffer[size];
	T data;
public:
	operator T() { return data; }
	T operator->() { return data; }
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