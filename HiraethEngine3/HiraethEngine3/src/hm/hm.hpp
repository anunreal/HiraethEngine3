#pragma once
#include "setup.hpp"
#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"
#include "mat4.hpp"
#include "quat.hpp"
#include "colour.hpp"
#include "matrixmath.hpp"
#include <string>
#include <iostream>

namespace hm {
    
    // to_string
    
    template<typename T>
    inline std::string to_string(const vec2<T>& vec) {
        return "vec2{" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + "}";
    };

	template<typename T>
	inline std::string to_string(const vec3<T>& vec) {
		return "vec3{" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + "}";
	}; 

	template<typename T>
	inline std::string to_string(const vec4<T>& vec) {
		return "vec4{" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ", " + std::to_string(vec.w) + "}";
	};

    inline std::string to_string(const colour& col) {
        return "colour{" + std::to_string(col.r) + ", " + std::to_string(col.g) + ", " + std::to_string(col.b) + ", " + std::to_string(col.a) + "}";
    };

	template<typename T>
	inline std::string to_string(const mat4<T>& mat) {
		return "mat4{" + 
			std::to_string(mat[0][0]) + ", " + std::to_string(mat[0][1]) + ", " + std::to_string(mat[0][2]) + ", " + std::to_string(mat[0][3]) + ",\n" +
			std::to_string(mat[1][0]) + ", " + std::to_string(mat[1][1]) + ", " + std::to_string(mat[1][2]) + ", " + std::to_string(mat[1][3]) + ",\n" +
			std::to_string(mat[2][0]) + ", " + std::to_string(mat[2][1]) + ", " + std::to_string(mat[2][2]) + ", " + std::to_string(mat[2][3]) + ",\n" +
			std::to_string(mat[3][0]) + ", " + std::to_string(mat[3][1]) + ", " + std::to_string(mat[3][2]) + ", " + std::to_string(mat[3][3]) + "}";

	};

	template<typename T>
	inline std::string to_string(const quat<T>& quat) {
		return "quat{" + std::to_string(quat.x) + ", " + std::to_string(quat.y) + ", " + std::to_string(quat.z) + ", " + std::to_string(quat.w) + "}";
	};
     
    
    // operator <<
    
    template<typename T>
    inline std::ostream& operator<<(std::ostream& os, const vec2<T>& vec) {
        os << to_string(vec);
        return os;
    };
    
	template<typename T>
	inline std::ostream& operator<<(std::ostream& os, const vec4<T>& vec) {
		os << to_string(vec);
		return os;
	};

    inline std::ostream& operator<<(std::ostream& os, const colour& col) {
        os << to_string(col);
        return os;
    };
    
	template<typename T>
	inline std::ostream& operator<<(std::ostream& os, const mat4<T>& mat) {
		os << to_string(mat);
		return os;
	};

	template<typename T>
	inline std::ostream& operator<<(std::iostream& os, const quat<T>& quat) {
		os << to_string(quat);
		return os;
	};

};