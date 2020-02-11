#pragma once
#include "setup.hpp"
#include "vec3.hpp"
#include "vec4.hpp"

namespace hm {

	template<typename T>
	struct mat4 {
		typedef vec4<T> col;

		col columns[4];

		mat4() { mat4(1.f); }
		
		mat4(const T v) {
			columns[0] = col(v, 0.f, 0.f, 0.f);
			columns[1] = col(0.f, v, 0.f, 0.f);
			columns[2] = col(0.f, 0.f, v, 0.f);
			columns[3] = col(0.f, 0.f, 0.f, v);
		};
		
		mat4(const T x0, const T y0, const T z0, const T w0,
			const T x1, const T y1, const T z1, const T w1,
			const T x2, const T y2, const T z2, const T w2,
			const T x3, const T y3, const T z3, const T w3) {
			columns[0] = col(x0, y0, z0, w0);
			columns[1] = col(x1, y1, z1, w1);
			columns[2] = col(x2, y2, z2, w2);
			columns[3] = col(x3, y3, z3, w3);
		};

		mat4(const col& v0, const col& v1, const col& v2, const col& v3) {
			columns[0] = v0;
			columns[1] = v1;
			columns[2] = v2;
			columns[3] = v3;
		};


		// accessors

		const col& operator[](const unsigned int index) const {
			if (index < 4)
				return columns[index];
			
			// Cannot access higher indices in this mat4 (size: 4, tried: index)
			assert(index < 4);
			return columns[0];
		};

		col& operator[](const unsigned int index) {
			if (index < 4)
				return columns[index];

			// Cannot access higher indices in this mat4 (size: 4, tried: index)
			assert(index < 4);
			return columns[0];
		};


		// operators

		mat4 operator*(const mat4& matrix) const {
			const col ia0 = columns[0];
			const col ia1 = columns[1];
			const col ia2 = columns[2];
			const col ia3 = columns[3];

			const col ib0 = matrix[0];
			const col ib1 = matrix[1];
			const col ib2 = matrix[2];
			const col ib3 = matrix[3];

			mat4 result;
			result[0] = ia0 * ib0[0] + ia1 * ib0[1] + ia2 * ib0[2] + ia3 * ib0[3];
			result[1] = ia0 * ib1[0] + ia1 * ib1[1] + ia2 * ib1[2] + ia3 * ib1[3];
			result[2] = ia0 * ib2[0] + ia1 * ib2[1] + ia2 * ib2[2] + ia3 * ib2[3];
			result[3] = ia0 * ib3[0] + ia1 * ib3[1] + ia2 * ib3[2] + ia3 * ib3[3];
			return result;
		};

		template<typename T>
		vec4<T> operator*(const vec4<T>& vector) const {
			vec4<T> result;
			result.x = vector.x * columns[0][0] + vector.y * columns[1][0] + vector.z * columns[2][0] + vector.w * columns[3][0];
			result.y = vector.x * columns[0][1] + vector.y * columns[1][1] + vector.z * columns[2][1] + vector.w * columns[3][1];
			result.z = vector.x * columns[0][2] + vector.y * columns[1][2] + vector.z * columns[2][2] + vector.w * columns[3][2];
			result.w = vector.x * columns[0][3] + vector.y * columns[1][3] + vector.z * columns[2][3] + vector.w * columns[3][3];
			
			if (result.w != 1.0f) {
				result.x /= result.w;
				result.y /= result.w;
				result.z /= result.w;
			}

			return result;
		};
	};

	typedef mat4<int> mat4i;
	typedef mat4<float> mat4f;
	typedef mat4<double> mat4d;
	
	template<typename T>
	static inline mat4<T> translate(const mat4<T>& matrix, const vec3<T>& position) {
		mat4 m = matrix;
		m[3] = matrix.columns[0] * position.x + matrix.columns[1] * position.y + matrix.columns[2] * position.z + matrix.columns[3];
		return m;
	};

	template<typename T>
	static inline mat4<T> translate(const mat4<T>& matrix, const vec2<T>& position) {
		return translate(matrix, vec3<T>(position.x, position.y, 0));
	};

	template<typename T>
	static inline mat4<T> scale(const mat4<T>& mat, const vec3<T>& factor) {
		mat4 mr = mat;
		mr[0] = mr[0] * factor.x;
		mr[1] = mr[1] * factor.y;
		mr[2] = mr[2] * factor.z;
		return mr;
	};

	template<typename T>
	static inline mat4<T> scale(const mat4<T>& mat, const vec2<T>& factor) {
		return scale(mat, vec3<T>(factor.x, factor.y, 1));
	};

	template<typename T>
	static mat4<T> rotate(const mat4<T>& matrix, const T angle, const vec3<T>& axisVector) {
		const float c = (float) std::cos(to_radians(angle));
		const float s = (float) std::sin(to_radians(angle));
		const float c1 = 1.0f - c;

		mat4 mr(0.0f);
		mr[3][3] = 1.0f;
		mr[0][0] = axisVector.x * axisVector.x * c1 + c;
		mr[0][1] = axisVector.x * axisVector.y * c1 + axisVector.z * s;
		mr[0][2] = axisVector.x * axisVector.z * c1 - axisVector.y * s;

		mr[1][0] = axisVector.x * axisVector.y * c1 - axisVector.z * s;
		mr[1][1] = axisVector.y * axisVector.y * c1 + c;
		mr[1][2] = axisVector.y * axisVector.z * c1 + axisVector.x * s;

		mr[2][0] = axisVector.x * axisVector.z * c1 + axisVector.y * s;
		mr[2][1] = axisVector.y * axisVector.z * c1 - axisVector.x * s;
		mr[2][2] = axisVector.z * axisVector.z * c1 + c;
		
		return matrix * mr;
	};

};