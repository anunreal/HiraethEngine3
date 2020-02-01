#pragma once
#include "mat4.hpp"

namespace hm {

	template<typename T>
	struct quat {
		T x, y, z, w;

		quat() : x(0), y(0), z(0), w(0) {};
		quat(const T x, const T y, const T z, const T w) : x(x), y(y), z(z), w(w) {};

		// operators

		inline quat operator+(const quat& q) const {
			return quat(q.x + x, q.y + y, q.z + z, q.w + w);
		};

	};

	typedef quat<int> quati;
	typedef quat<float> quatf;
	typedef quat<double> quatd;

	// turns this quaternion into a rotated transformation matrix
	template<typename T>
	static mat4<T> toMat4(const quat<T>& q) {
		mat4 result(1.0f);
		T qxx(q.x * q.x);
		T qyy(q.y * q.y);
		T qzz(q.z * q.z);
		T qxz(q.x * q.z);
		T qxy(q.x * q.y);
		T qyz(q.y * q.z);
		T qwx(q.w * q.x);
		T qwy(q.w * q.y);
		T qwz(q.w * q.z);

		result[0][0] = 1 - 2 * (qyy + qzz);
		result[0][1] = 2 * (qxy + qwz);
		result[0][2] = 2 * (qxz - qwy);

		result[1][0] = 2 * (qxy - qwz);
		result[1][1] = 1 - 2 * (qxx + qzz);
		result[1][2] = 2 * (qyz + qwx);

		result[2][0] = 2 * (qxz + qwy);
		result[2][1] = 2 * (qyz - qwx);
		result[2][2] = 1 - 2 * (qxx + qyy);
		return result;
	};

	// creates a quaternion from euler angles (in degrees)
	template<typename T>
	static quat<T> fromEuler(const vec3<T>& euler) {
		vec3<T> radians = to_radians(euler);

		T c1 = (T)std::cos(radians.y * 0.5);
		T c2 = (T)std::cos(radians.z * 0.5);
		T c3 = (T)std::cos(radians.x * 0.5);
		T s1 = (T)std::sin(radians.y * 0.5);
		T s2 = (T)std::sin(radians.z * 0.5);
		T s3 = (T)std::sin(radians.x * 0.5);
	
		return quat<T>(c1 * c3 * s2 + s1 * s3 * c2,
			s1 * c3 * c2 + c1 * s3 * s2,
			c1 * s3 * c2 - s1 * c3 * s2,
			c1 * c3 * c2 - s1 * s3 * s2);
	};

	// calculates the euler angles from this quaternion (in degrees)
	template<typename T>
	static vec3<T> toEuler(const quat<T>& quat) {
		vec3<T> euler;
		float sinr_cosp = 2.0f * (quat.w * quat.x + quat.y * quat.z);
		float cosr_cosp = 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y);
		euler.z = atan2(sinr_cosp, cosr_cosp);

		float sinp = 2.0f * (quat.w * quat.y - quat.z * quat.x);
		if (fabs(sinp) >= 1.0f)
			euler.x = (float)copysign(PI / 2, sinp);
		else
			euler.x = asin(sinp);

		float siny_cosp = 2.0f * (quat.w * quat.z + quat.x * quat.y);
		float cosy_cosp = 1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z);
		euler.y = atan2(siny_cosp, cosy_cosp);
		return euler;
	};

};