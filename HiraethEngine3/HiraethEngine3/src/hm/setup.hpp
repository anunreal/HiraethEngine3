#pragma once
#include <cmath>
#include <assert.h>
#include <cstdint>

namespace hm {

	constexpr double PI = 3.14159265358979323846;

	template<typename T>
	inline double to_radians(const T value) {
		return value / 180. * PI;
	};

	template<typename T>
	inline int sign(const T value) {
		int v = 0;
		if (value < 0)
			v = -1;
		else if (value > 0)
			v = 1;

		return v;
	};

	template<typename T>
	inline T clamp(const T val, const T l, const T r) {
		if (val < l)
			return l;
		if (val > r)
			return r;
		return val;
	};

};