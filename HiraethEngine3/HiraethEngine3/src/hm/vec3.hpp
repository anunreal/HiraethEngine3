#pragma once
#include "setup.hpp"

namespace hm {

	template<typename T>
	struct vec3 {
		T x, y, z;

		vec3() : x(0), y(0), z(0) {};
		vec3(T v) : x(v), y(v), z(v) {};
		vec3(T x, T y, T z) : x(x), y(y), z(z) {};
		vec3(const vec3& v) : x(v.x), y(v.y), z(v.z) {};


		// operators

		inline vec3 operator*(const vec3& v) const {
			return vec3(x * v.x, y * v.y, z * v.z);
		};

		inline vec3 operator+(const vec3& v) const {
			return vec3(x + v.x, y + v.y, z - v.z);
		};

		inline vec3 operator-(const vec3& v) const {
			return vec3(x - v.x, y - v.y, z - v.z);
		};

		inline vec3 operator/(const vec3& v) const {
			return vec3(x / v.x, y / v.y, z / v.z);
		};

		inline vec3 operator-() const {
			return vec3(-x, -y, -z);
		};

		inline void operator+=(const vec3& v) {
			x += v.x;
			y += v.y;
			z += v.z;
		};
	
		inline void operator-=(const vec3& v) {
			x -= v.x;
			y -= v.y;
			z -= v.z;
		};
	};

	typedef vec3<int> vec3i;
	typedef vec3<float> vec3f;
	typedef vec3<double> vec3d;

	template<typename T>
	static inline vec3<T> to_radians(const vec3<T> vector) {
		return vec3<T>(to_radians(vector.x), to_radians(vector.y), to_radians(vector.z));
	};

};