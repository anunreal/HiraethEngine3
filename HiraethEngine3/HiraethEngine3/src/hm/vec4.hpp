#pragma once
#include <assert.h>

#include "vector.hpp"

namespace hm {
    
    template<typename T>
    struct vec<4, T> {
        T x, y, z, w;
        
        vec() : x(0), y(0), z(0), w(0) {};
        vec(T const v) : x(v), y(v), z(v), w(v) {};
        vec(T const x, T const y, T const z, T const w) : x(x), y(y), z(z), w(w) {};
        template<typename T1>
        vec(vec<3, T1> const& vec, T1 const w = 1.0f) : x((T) vec.x), y((T) vec.y), z((T) vec.z), w(w) {};
        template<typename T1>
        vec(vec<4, T1> const& vec) : x(vec.x), y(vec.y), z(vec.z), w(vec.w) {};
        
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
        
        vec operator*(float const value) const {
            return vec(x * value, y * value, z * value, w * value);
        };

        vec operator*(vec const& v) const {
            return vec(x * v.x, y * v.y, z * v.z, w * v.w);
        };
        
        vec operator+(vec const& v) const {
            return vec(x + v.x, y + v.y, z + v.z, w + v.w);
        };        

        vec operator-(vec const& v) const {
            return vec(x - v.x, y - v.y, z - v.z, w - v.w);
        };
    };
    
    typedef vec<4, float> vec4f;
    typedef vec<4, double> vec4d;
    typedef vec<4, int32_t> vec4i;
};
