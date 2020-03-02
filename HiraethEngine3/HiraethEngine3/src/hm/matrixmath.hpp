#pragma once
#include "quat.hpp"
#include "mat4.hpp"

namespace hm {
    template<typename T>
        static mat4<T> createOrthographicProjectionMatrix(const vec2<T>& size, const vec2<T>& center, const T depth) {
        
        T size2x = size.x / 2;
        T size2y = size.y / 2;
        
        // create projection matrix bounding box
        // right, left, top, bottom, far, near
        const float r = (center.x + size2x) - 1.0f,
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
    
    template<typename T>
        static mat4<T> createPerspectiveProjectionMatrix(const T fov, const T ratio, const T nearPlane, const T farPlane) {
        
        mat4<T> mat(0.0);
        const T frustumLength = farPlane - nearPlane;
        const T tanHalfAngle = (T)std::tan(to_radians(fov) / 2.0);
        
        mat[0][0] = 1 / (ratio * tanHalfAngle);
        mat[1][1] = 1 / tanHalfAngle;
        mat[2][2] = (T)-((farPlane + nearPlane) / frustumLength);
        mat[2][3] = -1;
        mat[3][2] = (T)-((2 * nearPlane * farPlane) / frustumLength);
        return mat;
        
    };
    
    template<typename T>
        static mat4<T> createViewMatrix(const vec3<T>& position, const vec3<T>& rotation) {
        
        mat4<T> mat(1.0);
        mat = rotate(mat, rotation.x, hm::vec3f(1, 0, 0));
        mat = rotate(mat, rotation.y, hm::vec3f(0, 1, 0));
        mat = translate(mat, -position);
        return mat;
        
    };
    
    template<typename T>
        static mat4<T> createTransformationMatrix(const vec3<T>& position, const quat<T>& rotation, const vec3<T>& scale) {
        
        mat4<T> p(1);
        p = hm::translate(p, position);
        mat4<T> s(1);
        s = hm::scale(s, scale);
        mat4<T> r = hm::toMat4(rotation);
        return p * r * s;
        
    };
    
    template<typename T>
        static mat4<T> createTransformationMatrix(const vec3<T>& position, const vec3<T>& rotation, const vec3<T>& scale) {
        
        mat4<T> m(1);
        m = hm::scale(m, scale);
        m = hm::translate(m, position);
        m = hm::rotate(m, rotation);
        return m;
        
    };
    
    template<typename T>
        static mat4<T> createTransformationMatrix(const vec2<T>& position, const vec2<T>& scale) {
        
        mat4<T> mat(1);
        mat = hm::scale(mat, scale);
        mat = hm::translate(mat, position);
        return mat;
        
    };
    
    template<typename T>
        static vec3<T> getDirectionVectorFromRotation(const vec3<T>& eulerAngles) {
        
        T rx = to_radians(eulerAngles.x);
        T ry = to_radians(eulerAngles.y);
        
        vec3<T> direction;
        direction.x = (T) (std::cos(ry) * std::cos(rx)); 
        direction.y = (T) std::sin(rx); 
        direction.z = (T) (std::cos(ry) * std::cos(rx)); 
        return direction;
        
    };
    
};