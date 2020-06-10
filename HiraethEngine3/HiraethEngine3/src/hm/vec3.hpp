#ifndef HM_VEC3_HPP
#define HM_VEC3_HPP

#include "setup.hpp"
#include "vector.hpp"
#include <string>

namespace hm {    
    template<typename T>
    struct vec<3, T> {
        T x, y, z;
        
        vec() : x(0), y(0), z(0) {};
        vec(T v) : x(v), y(v), z(v) {};
        vec(T x, T y, T z) : x(x), y(y), z(z) {};
        template<typename T1>
        vec(vec<2, T1> const& vec, T1 const z = 1) : x((T) vec.x), y((T) vec.y), z((T) z) {};
        template<typename T1>
        vec(vec<3, T1> const& v) : x((T) v.x), y((T) v.y), z((T) v.z) {};
        template<typename T1>
        vec(vec<4, T1> const& v) : x((T) v.x), y((T) v.y), z((T) v.z) {};
        
        // accessors
        
        T const& operator[](uint8_t const index) const {
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
        
        
        T& operator[](uint8_t const index) {
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
        
        inline vec operator*(vec const& v) const {
            return vec(x * v.x, y * v.y, z * v.z);
        };
        
        inline vec operator+(vec const& v) const {
            return vec(x + v.x, y + v.y, z + v.z);
        };
        
        inline vec operator-(vec const& v) const {
            return vec(x - v.x, y - v.y, z - v.z);
        };
        
        inline vec operator/(vec const& v) const {
            return vec(x / v.x, y / v.y, z / v.z);
        };
        
        inline vec operator-() const {
            return vec(-x, -y, -z);
        };
        
        inline vec& operator*=(float const f) {
            x *= f;
            y *= f;
            z *= f;
            return *this;
        };

        inline void operator+=(vec const& v) {
            x += v.x;
            y += v.y;
            z += v.z;
        };
        
        inline void operator-=(vec const& v) {
            x -= v.x;
            y -= v.y;
            z -= v.z;
        };
    };
    
    typedef vec<3, float> vec3f;
    typedef vec<3, double> vec3d;
    typedef vec<3, int32_t> vec3i;
    
    template<typename T>
    static inline vec<3, T> length(vec<3, T> const& vector) {
        return std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
    };
    
    template<typename T>
    static inline vec<3, T> to_radians(vec<3, T> const& vector) {
        return vec<3, T>(to_radians(vector.x), to_radians(vector.y), to_radians(vector.z));
    };
    
    template<typename T>
    static inline vec<3, T> normalize(vec<3, T> const& vector) {
        return vector / length(vector);
    };
    
    template<typename T>
    static inline vec<3, T> cross(vec<3, T> const& left, vec<3, T> const& right) {
        return vec<3, T>(left.y * right.z - left.z * right.y,
                       left.z * right.x - left.x * right.z,
                       left.x * right.y - left.y * right.x);
    };
    
    // parses a vec3 from a string. The coordinates should be split by a single '/' char
    static inline vec3f parseVec3f(std::string const& input) {
        std::string arguments[3];
        size_t index0 = input.find('/');
        arguments[0] = input.substr(0, index0);
        
        // we need the actual offset, find will only give us the offset in the substr (add 1 for the last /)
        size_t index1 = input.substr(index0 + 1).find('/') + index0 + 1;
        arguments[1] = input.substr(index0 + 1, index1);
        arguments[2] = input.substr(index1 + 1);
        
        return vec3f(std::stof(arguments[0]), std::stof(arguments[1]), std::stof(arguments[2]));
    };
    
};

#endif
