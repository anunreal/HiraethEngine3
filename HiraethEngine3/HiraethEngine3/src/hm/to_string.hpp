#ifndef HM_TO_STRING_HPP
#define HM_TO_STRING_HPP

#include <string>
#include <iostream>

namespace hm {
    // to_string
    
    template<typename T>
        inline std::string to_string(vec2<T> const& vec) {
        return "vec2{" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + "}";
    };
    
    template<typename T>
        inline std::string to_string(vec3<T> const& vec) {
        return "vec3{" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + "}";
    };
    
    template<typename T>
        inline std::string to_string( vec4<T> const& vec) {
        return "vec4{" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ", " + std::to_string(vec.w) + "}";
    };
    
    inline std::string to_string(colour const& col) {
        return "colour{" + std::to_string(col.r) + ", " + std::to_string(col.g) + ", " + std::to_string(col.b) + ", " + std::to_string(col.a) + " * " + std::to_string(col.i) + "}";
    };
    
    template<typename T>
        inline std::string to_string(mat<3, 3, T> const& mat) {
        return "mat3{" + std::to_string(mat[0][0]) + ", " + std::to_string(mat[0][1]) + ", " + std::to_string(mat[0][2]) + ",\n" +
            std::to_string(mat[1][0]) + ", " + std::to_string(mat[1][1]) + ", " + std::to_string(mat[1][2]) + ",\n" +
            std::to_string(mat[2][0]) + ", " + std::to_string(mat[2][1]) + ", " + std::to_string(mat[2][2]) + "}";
    };
    
    template<typename T>
        inline std::string to_string(mat<4, 4, T> const& mat) {
        return "mat4{" +
            std::to_string(mat[0][0]) + ", " + std::to_string(mat[0][1]) + ", " + std::to_string(mat[0][2]) + ", " + std::to_string(mat[0][3]) + ",\n" +
            std::to_string(mat[1][0]) + ", " + std::to_string(mat[1][1]) + ", " + std::to_string(mat[1][2]) + ", " + std::to_string(mat[1][3]) + ",\n" +
            std::to_string(mat[2][0]) + ", " + std::to_string(mat[2][1]) + ", " + std::to_string(mat[2][2]) + ", " + std::to_string(mat[2][3]) + ",\n" +
            std::to_string(mat[3][0]) + ", " + std::to_string(mat[3][1]) + ", " + std::to_string(mat[3][2]) + ", " + std::to_string(mat[3][3]) + "}";
        
    };
    
    template<typename T>
        inline std::string to_string(quat<T> const& quat) {
        return "quat{" + std::to_string(quat.x) + ", " + std::to_string(quat.y) + ", " + std::to_string(quat.z) + ", " + std::to_string(quat.w) + "}";
    };
    
    
    // operator <<
    
    template<typename T>
        inline std::ostream& operator<<(std::ostream& os, vec2<T> const& vec) {
        os << to_string(vec);
        return os;
    };
    
    template<typename T>
        inline std::ostream& operator<<(std::ostream& os, vec4<T> const& vec) {
        os << to_string(vec);
        return os;
    };
    
    inline std::ostream& operator<<(std::ostream& os, colour const& col) {
        os << to_string(col);
        return os;
    };
    
    template<typename T>
        inline std::ostream& operator<<(std::ostream& os, mat<3, 3, T> const& mat) {
        os << to_string(mat);
        return os;
    };
    
    template<typename T>
        inline std::ostream& operator<<(std::ostream& os, mat<4, 4, T> const& mat) {
        os << to_string(mat);
        return os;
    };
    
    template<typename T>
        inline std::ostream& operator<<(std::iostream& os, quat<T> const& quat) {
        os << to_string(quat);
        return os;
    };
};

#endif