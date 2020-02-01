#pragma once
#include <assert.h>

namespace hm {

	template<typename T>
	struct vec4 {
		T x, y, z, w;

		vec4() : x(0), y(0), z(0), w(0) {};
		vec4(const T v) : x(v), y(v), z(v), w(v) {};
		vec4(const T x, const T y, const T z, const T w) : x(x), y(y), z(z), w(w) {};

		// accessors

		const T& operator[](const unsigned int index) const {
			switch (index) {
			case 0:
				return x;

			case 1:
				return y;
				
			case 2:
				return z;

			case 3:
				return w;
			}

			// Cannot access higher indices in this vec4 (size: 4, tried: index)
			assert(index < 4);
			return x;
		};


		T& operator[](const unsigned int index) {
			switch (index) {
			case 0:
				return x;

			case 1:
				return y;

			case 2:
				return z;

			case 3:
				return w;
			}

			// Cannot access higher indices in this vec4 (size: 4, tried: index)
			assert(index < 4);
			return x;
		};

		// operators

		vec4 operator*(const float value) const {
			return vec4(x * value, y * value, z * value, w * value);
		};

		vec4 operator+(const vec4& vec) const {
			return vec4(x + vec.x, y + vec.y, z + vec.z, w + vec.w);
		};

	};


	typedef vec4<int> vec4i;
	typedef vec4<float> vec4f;
	typedef vec4<double> vec4d;
};