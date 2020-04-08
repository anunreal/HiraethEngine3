#ifndef HM_MAT3_HPP
#define HM_MAT3_HPP

#include "matrix.hpp"
#include "vec3.hpp"

namespace hm {
    template<typename T>
        struct mat<3, 3, T> {
        typedef vec3<T> col;
        
        col columns[3];
        
        mat() { mat(1.f); };
        
        mat(const T v) : columns{col(v, 0.f, 0.f), col(0.f, v, 0.f), col(0.f, 0.f, v)} {};
        
        mat(const T x0, const T y0, const T z0,
            const T x1, const T y1, const T z1,
            const T x2, const T y2, const T z2) :
        columns{
            col(x0, y0, z0),
            col(x1, y1, z1),
            col(x2, y2, z2)
        }
        {};
        
        mat(const col& v0, const col& v1, const col& v2) :
        columns{
            (v0),
            (v1),
            (v2)
        }
        {};
        
        mat(const T* ptr) :
        columns{
            col(ptr[0], ptr[1], ptr[2]),
            col(ptr[3], ptr[4], ptr[5]),
            col(ptr[6], ptr[7], ptr[8])
        }
        {};
        
        // conversions
        
        template<typename T>
            mat(mat<4, 4, T> const& mat) :
        columns{
            col(mat[0][0], mat[0][1], mat[0][2]),
            col(mat[1][0], mat[1][1], mat[1][2]),
            col(mat[2][0], mat[2][1], mat[2][2])
        }
        {};
        
        // accessors
        
        const col& operator[](const uint8_t index) const {
            if(index < 3)
                return columns[index];
            
            // Cannot access higher indices in this mat3 (size: 3, tried: index)
            assert(index < 3);
            return columns[0];
        };
        
        col& operator[](const uint8_t index) {
            if(index < 3)
                return columns[index];
            
            // Cannot access higher indices in this mat3 (size: 3, tried: index)
            assert(index < 3);
            return columns[0];
        };
        
        
        // operators
        
        mat operator*(const mat& matrix) const {
            
            const col ia0 = columns[0];
            const col ia1 = columns[1];
            const col ia2 = columns[2];
            
            const col ib0 = matrix[0];
            const col ib1 = matrix[1];
            const col ib2 = matrix[2];
            
            mat result;
            result[0] = ia0 * ib0[0] + ia1 * ib0[1] + ia2 * ib0[2];
            result[1] = ia0 * ib1[0] + ia1 * ib1[1] + ia2 * ib1[2];
            result[2] = ia0 * ib2[0] + ia1 * ib2[1] + ia2 * ib2[2];
            return result;
            
        };
        
        template<typename T>
            vec3<T> operator*(const vec3<T>& vector) const {
            
            vec3<T> result;
            result.x = vector.x * columns[0][0] + vector.y * columns[1][0] + vector.z * columns[2][0];
            result.y = vector.x * columns[0][1] + vector.y * columns[1][1] + vector.z * columns[2][1];
            result.z = vector.x * columns[0][2] + vector.y * columns[1][2] + vector.z * columns[2][2];
            return result;
            
        };
        
    };
    
    typedef mat<3, 3, float> mat3f;
    typedef mat<3, 3, double> mat3d;
    typedef mat<3, 3, int32_t> mat3i;
    
    template<typename T>
        static mat<3, 3, T> transpose(mat<3, 3, T> const& matrix) {
        
        mat<3, 3, T> result;
        
        result[0][0] = matrix[0][0];
        result[0][1] = matrix[1][0];
        result[0][2] = matrix[2][0];
        result[1][0] = matrix[0][1];
        result[1][1] = matrix[1][1];
        result[1][2] = matrix[2][1];
        result[2][0] = matrix[0][2];
        result[2][1] = matrix[1][2];
        result[2][2] = matrix[2][2];
        
        return result;
        
    };
    
    template<typename T>
        static mat<3, 3, T> inverse(mat<3, 3, T> const& matrix) {
        
        T determinant = static_cast<T>(1) / (+matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[2][1] * matrix[1][2])
                                             -matrix[1][0] * (matrix[0][1] * matrix[2][2] - matrix[2][1] * matrix[0][2])
                                             +matrix[2][0] * (matrix[0][1] * matrix[1][2] - matrix[1][1] * matrix[0][2]));
        
        mat<3, 3, T> inverse;
        inverse[0][0] = +(matrix[1][1] * matrix[2][2] - matrix[2][1] * matrix[1][2]) * determinant;
        inverse[1][0] = -(matrix[1][0] * matrix[2][2] - matrix[2][0] * matrix[1][2]) * determinant;
        inverse[2][0] = +(matrix[1][0] * matrix[2][1] - matrix[2][0] * matrix[1][1]) * determinant;
        inverse[0][1] = -(matrix[0][1] * matrix[2][2] - matrix[2][1] * matrix[0][2]) * determinant;
        inverse[1][1] = +(matrix[0][0] * matrix[2][2] - matrix[2][0] * matrix[0][2]) * determinant;
        inverse[2][1] = -(matrix[0][0] * matrix[2][1] - matrix[2][0] * matrix[0][1]) * determinant;
        inverse[0][2] = +(matrix[0][1] * matrix[1][2] - matrix[1][1] * matrix[0][2]) * determinant;
        inverse[1][2] = -(matrix[0][0] * matrix[1][2] - matrix[1][0] * matrix[0][2]) * determinant;
        inverse[2][2] = +(matrix[0][0] * matrix[1][1] - matrix[1][0] * matrix[0][1]) * determinant;
        return inverse;
        
    };
    
};

#endif