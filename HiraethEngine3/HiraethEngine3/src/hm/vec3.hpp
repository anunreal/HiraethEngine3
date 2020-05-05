#pragma once
#include "setup.hpp"
#include "vec2.hpp"
#include <string>

namespace hm {
	
	template<typename T>
	struct vec3 {
		T x, y, z;
		
		vec3() : x(0), y(0), z(0) {};
		vec3(T v) : x(v), y(v), z(v) {};
		vec3(T x, T y, T z) : x(x), y(y), z(z) {};
		vec3(vec3 const& v) : x(v.x), y(v.y), z(v.z) {};
		
		// conversions
		
		template<typename T1>
		vec3(vec2<T1> const& vec, float const z = 1.f) : x(vec.x), y(vec.y), z(z) {};
		
		// accessors
		
		const T& operator[](const unsigned int index) const {
			switch (index) {
			case 0:
				return x;
				
			case 1:
				return y;
				
			case 2:
				return z;
			}
			
			// Cannot access higher indices in this vec4 (size: 4, tried: index)
			assert(index < 3);
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
			}
			
			// Cannot access higher indices in this vec4 (size: 4, tried: index)
			assert(index < 3);
			return x;
		};
		
		
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
	
	typedef vec3<float> vec3f;
	typedef vec3<double> vec3d;
	typedef vec3<int32_t> vec3i;
	
	template<typename T>
	static inline vec3<T> length(const vec3<T>& vector) {
		return std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
	};
	
	template<typename T>
	static inline vec3<T> to_radians(const vec3<T>& vector) {
		return vec3<T>(to_radians(vector.x), to_radians(vector.y), to_radians(vector.z));
	};
	
	template<typename T>
	static inline vec3<T> normalize(const vec3<T>& vector) {
		return vector / length(vector);
	};
	
	template<typename T>
	static inline vec3<T> cross(const vec3<T>& left, const vec3<T>& right) {
		return vec3<T>(left.y * right.z - left.z * right.y,
					   left.z * right.x - left.x * right.z,
					   left.x * right.y - left.y * right.x);
	};
	
	// parses a vec3 from a string. The coordinates should be split by a single '/' char
	static inline vec3f parseVec3f(const std::string& input) {
		std::string arguments[3];
		size_t index0 = input.find('/');
		arguments[0] = input.substr(0, index0);
		
		// we need the actual offset, find will only give us the offset in the substr (add 1 for the last /)
		size_t index1 = input.substr(index0 + 1).find('/') + index0 + 1;
		arguments[1] = input.substr(index0 + 1, index1);
		arguments[2] = input.substr(index1 + 1);
		
		return vec3(std::stof(arguments[0]), std::stof(arguments[1]), std::stof(arguments[2]));
	};
	
};
