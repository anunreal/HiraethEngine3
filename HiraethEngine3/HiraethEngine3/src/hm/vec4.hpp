#pragma once
#include <assert.h>

#include "vec3.hpp"

namespace hm {
    
    template<typename T>
        struct vec4 {
        T x, y, z, w;
        
        vec4() : x(0), y(0), z(0), w(0) {};
        vec4(T const v) : x(v), y(v), z(v), w(v) {};
        vec4(T const x, T const y, T const z, T const w) : x(x), y(y), z(z), w(w) {};
        
        //conversions
        
        template<typename T1>
            vec4(vec3<T1> const& vec, float const w = 1.0f) : x((T) vec.x), y((T) vec.y), z((T) vec.z), w(w) {};
        
        // accessors
        
        const T& operator[](uint8_t const index) const {
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
        
        
        T& operator[](uint8_t const index) {
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
        
        vec4 operator*(float const value) const {
            return vec4(x * value, y * value, z * value, w * value);
        };
        
        vec4 operator+(vec4 const& vec) const {
            return vec4(x + vec.x, y + vec.y, z + vec.z, w + vec.w);
        };
        
    };
    
    typedef vec4<float> vec4f;
    typedef vec4<double> vec4d;
    typedef vec4<int32_t> vec4i;
};