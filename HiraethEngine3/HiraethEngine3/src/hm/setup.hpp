#pragma once
#include <cmath>
#include <assert.h>
#include <cstdint>

namespace hm {
    
    constexpr double PI = 3.14159265358979323846;
    typedef int32_t length_t;

    // degrees to radians
    template<typename T>
    inline T to_radians(const T value) {
        return (T) (value / 180. * PI);
    };

    // radians to degrees
    template<typename T>
    inline T to_degrees(const T value) {
        return (T) (value / PI * 180);
    };

    // returns -1, 0 or 1, depending on the sign of value
    template<typename T>
    inline int sign(const T value) {
        int v = 0;
        if (value < 0)
            v = -1;
        else if (value > 0)
            v = 1;
        
        return v;
    };

    // clamps a value between l and r
    template<typename T>
    inline T clamp(const T val, const T l, const T r) {
        if (val < l)
            return l;
        if (val > r)
            return r;
        return val;
    };

    // returns the nearest power of two that is less or equal to x
    inline uint32_t floorPowerOfTwo(uint32_t x) {
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x ^ (x >> 1);
    };

    // returns the nearest power of two that is equal or greater to x
    inline uint32_t ceilPowerOfTwo(uint32_t x) {
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x + 1;
    };
};
