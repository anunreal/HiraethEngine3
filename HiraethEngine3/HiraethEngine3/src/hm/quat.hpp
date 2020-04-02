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
    
    typedef quat<float> quatf;
    typedef quat<double> quatd;
    typedef quat<int32_t> quati;
    
    // turns this quaternion into a rotated transformation matrix
    template<typename T>
        static mat<4, 4, T> toMat4(const quat<T>& q) {
        mat<4, 4, T> result(1.0f);
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
    
    // creates a quaternion from euler angles (in radians)
    template<typename T>
        static quat<T> fromEulerRadians(const vec3<T>& radians) {
        
        T cx = (T)std::cos(radians.x * 0.5);
        T cy = (T)std::cos(radians.y * 0.5);
        T cz = (T)std::cos(radians.z * 0.5);
        T sx = (T)std::sin(radians.x * 0.5);
        T sy = (T)std::sin(radians.y * 0.5);
        T sz = (T)std::sin(radians.z * 0.5);
        
        return quat<T>(sx * cy * cz - cx * sy * sz,
                       cx * sy * cz + sx * cy * sz,
                       cx * cy * sz - sx * sy * cz,
                       cx * cy * cz + sx * sy * sz);
    };
    
    // creates a quaternion from euler angles (in degrees)
    template<typename T>
        static inline quat<T> fromEulerDegrees(const vec3<T>& euler) {
        return fromEulerRadians(to_radians(euler));
    }
    
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
    
    static quatf parseQuatf(const std::string& input) {
        
        std::string arguments[4];
        size_t index0 = input.find('/');
        arguments[0] = input.substr(0, index0);
        
        // we need the actual offset, find will only give us the offset in the substr (add 1 for the last /)
        size_t index1 = input.substr(index0 + 1).find('/') + index0 + 1;
        arguments[1] = input.substr(index0 + 1, index1);
        
        size_t index2 = input.substr(index1 + 1).find('/') + index1 + 1;
        arguments[2] = input.substr(index1 + 1, index2);
        arguments[3] = input.substr(index2 + 1);
        
        return quatf(std::stof(arguments[0]), std::stof(arguments[1]), std::stof(arguments[2]), std::stof(arguments[3]));
        
    };
    
};