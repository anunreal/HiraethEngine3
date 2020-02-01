#pragma once
#include "setup.hpp"
#include "vec3.hpp"
#include "vec4.hpp"

namespace hm {

	struct mat4 {
		vec4f columns[4];

		mat4() { mat4(1.f); }
		
		mat4(const float v) {
			columns[0] = vec4f(v, 0.f, 0.f, 0.f);
			columns[1] = vec4f(0.f, v, 0.f, 0.f);
			columns[2] = vec4f(0.f, 0.f, v, 0.f);
			columns[3] = vec4f(0.f, 0.f, 0.f, v);
		};
		
		mat4(const float x0, const float y0, const float z0, const float w0,
			const float x1, const float y1, const float z1, const float w1,
			const float x2, const float y2, const float z2, const float w2,
			const float x3, const float y3, const float z3, const float w3) {
			columns[0] = vec4f(x0, y0, z0, w0);
			columns[1] = vec4f(x1, y1, z1, w1);
			columns[2] = vec4f(x2, y2, z2, w2);
			columns[3] = vec4f(x3, y3, z3, w3);
		};

		mat4(const vec4f& v0, const vec4f& v1, const vec4f& v2, const vec4f& v3) {
			columns[0] = v0;
			columns[1] = v1;
			columns[2] = v2;
			columns[3] = v3;
		};


		// accessors

		const vec4f& operator[](const unsigned int index) const {
			if (index < 4)
				return columns[index];
			
			// Cannot access higher indices in this mat4 (size: 4, tried: index)
			assert(index < 4);
			return columns[0];
		};

		vec4f& operator[](const unsigned int index) {
			if (index < 4)
				return columns[index];

			// Cannot access higher indices in this mat4 (size: 4, tried: index)
			assert(index < 4);
			return columns[0];
		};


		// operators

		mat4 operator*(const mat4& matrix) const {
			const vec4f ia0 = columns[0];
			const vec4f ia1 = columns[1];
			const vec4f ia2 = columns[2];
			const vec4f ia3 = columns[3];

			const vec4f ib0 = matrix[0];
			const vec4f ib1 = matrix[1];
			const vec4f ib2 = matrix[2];
			const vec4f ib3 = matrix[3];

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

	static inline mat4 translate(const mat4& matrix, const vec3f& position) {
		mat4 m = matrix;
		m[3] = matrix.columns[0] * position.x + matrix.columns[1] * position.y + matrix.columns[2] * position.z + matrix.columns[3];
		return m;
	};

	static inline mat4 scale(const mat4& mat, const vec3f& factor) {
		mat4 mr = mat;
		mr[0] = mr[0] * factor.x;
		mr[1] = mr[1] * factor.y;
		mr[2] = mr[2] * factor.z;
		return mr;
	};

	static mat4 rotate(const mat4& matrix, const float angle, const vec3f& axisVector) {
		const float c = (float) std::cos(radians(angle));
		const float s = (float) std::sin(radians(angle));
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

	template<typename T>
	static mat4 createOrthographic(const vec2<T>& size, const vec2<T>& center, const T depth) {
		
		T size2x = size.x / 2;
		T size2y = size.y / 2;

		// create projection matrix bounding box
		// right, left, top, bottom, far, near
		const float 
			r = (center.x + size2x) - 1.0f,
			l = center.x - size2x,
			b = (center.y + size2y) - 1.0f,
			t = center.y - size2y,
			f = depth,
			n = -1.0f;

		hm::mat4 orthoMat = hm::mat4(1.0f);
		orthoMat[0][0] = 2.0f / (r - l);
		orthoMat[1][1] = 2.0f / (t - b);
		orthoMat[2][2] = 2.0f / (f - n);
		orthoMat[3][0] = -(r + l) / (r - l);
		orthoMat[3][1] = -(t + b) / (t - b);
		orthoMat[3][2] = -(f + n) / (f - n);

		return orthoMat;
	};
};