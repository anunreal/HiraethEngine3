#pragma once

namespace hm {

	template<typename T>
	struct vec3 {
		T x, y, z;

		vec3() : x(0), y(0), z(0) {};
		vec3(T v) : x(v), y(v), z(v) {};
		vec3(T x, T y, T z) : x(x), y(y), z(z) {};
		vec3(const vec3& v) : x(v.x), y(v.y), z(v.z) {};


		// operators

		vec3 operator*(const vec3& v) {
			return vec3(x * v.x, y * v.y, z * v.z);
		};

		vec3 operator+(const vec3& v) {
			return vec3(x + v.x, y + v.y, z - v.z);
		};

		vec3 operator-(const vec3& v) {
			return vec3(x - v.x, y - v.y, z - v.z);
		};

		vec3 operator/(const vec3& v) {
			return vec3(x / v.x, y / v.y, z / v.z);
		};
	};

	typedef vec3<int> vec3i;
	typedef vec3<float> vec3f;
	typedef vec3<double> vec3d;
};