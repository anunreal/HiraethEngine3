#ifndef HM_MATRIXMATH_HPP
#define HM_MATRIXMATH_HPP

#include "quat.hpp"
#include "mat4.hpp"
#include <iostream>

namespace hm {
    template<typename T>
    static mat<4, 4, T> createOrthographicProjectionMatrix(vec<2, T> const& size, vec<2, T> const& center, T const depth) {        
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
        
        hm::mat<4, 4, T> orthoMat(1.0f);
        orthoMat[0][0] = 2.0f / (r - l);
        orthoMat[1][1] = 2.0f / (t - b);
        orthoMat[2][2] = 2.0f / (f - n);
        orthoMat[3][0] = -(r + l) / (r - l);
        orthoMat[3][1] = -(t + b) / (t - b);
        orthoMat[3][2] = -(f + n) / (f - n);
        
        return orthoMat;        
    };
    
    template<typename T>
    static mat<4, 4, T> createPerspectiveProjectionMatrix(T const fov, T const ratio, T const nearPlane, T const farPlane) {        
        mat<4, 4, T> mat(static_cast<T>(1));
        T const frustumLength = farPlane - nearPlane;
        T const tanHalfAngle = (T)std::tan(to_radians(fov / static_cast<T>(2)));

        T const y_scale = static_cast<T>(1) / tanHalfAngle;
        T const x_scale = y_scale / ratio;
        
        mat[0][0] = x_scale;
        mat[1][1] = y_scale;
        mat[2][3] = static_cast<T>(-1);
        mat[2][2] = (T)-((farPlane + nearPlane) / frustumLength);
        mat[3][2] = (T)-((static_cast<T>(2) * nearPlane * farPlane) / frustumLength);
        mat[3][3] = static_cast<T>(0);
        return mat;        
    };
    
    template<typename T>
    static mat<4, 4, T> createViewMatrix(vec<3, T> const& position, vec<3, T> const& rotation) {        
        mat<4, 4, T> mat(static_cast<T>(1));
        mat = rotate(mat, rotation.x, hm::vec3f(1, 0, 0));
        mat = rotate(mat, rotation.y, hm::vec3f(0, 1, 0));
        mat = translate(mat, -position);
        return mat;        
    };
    
    template<typename T>
    static mat<4, 4, T> createTransformationMatrix(vec<3, T> const& position, quat<T> const& rotation, vec<3, T> const& scale) {        
        mat<4, 4, T> p(static_cast<T>(1));
        p = hm::translate(p, position);
        mat<4, 4, T> s(static_cast<T>(1));
        s = hm::scale(s, scale);
        mat<4, 4, T> r = hm::toMat4(rotation);
        return p * r * s;        
    };
    
    template<typename T>
    static mat<4, 4, T> createTransformationMatrix(vec<3, T> const& position, vec<3, T> const& rotation, vec<3, T> const& scale) {        
        mat<4, 4, T> m(static_cast<T>(1));
        m = hm::scale(m, scale);
        m = hm::translate(m, position);
        m = hm::rotate(m, rotation);
        return m;        
    };
    
    template<typename T>
    static mat<4, 4, T> createTransformationMatrix(vec<2, T> const& position, vec<2, T> const& scale) {        
        mat<4, 4, T> mat(static_cast<T>(1));
        mat = hm::translate(mat, position);
        mat = hm::scale(mat, scale);
        return mat;        
    };
    
    template<typename T>
    static vec<3, T> getDirectionVectorFromRotation(vec<3, T> const& eulerAngles) {
        T rx = to_radians(eulerAngles.x);
        T ry = to_radians(eulerAngles.y);
        
        vec<3, T> direction;
        direction.x = (T) (std::cos(ry) * std::cos(rx));
        direction.y = (T) std::sin(rx);
        direction.z = (T) (std::cos(ry) * std::cos(rx));
        return direction;        
    };    
};

#endif
