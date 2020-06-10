#ifndef HM_MAT4_HPP
#define HM_MAT4_HPP

#include "matrix.hpp"
#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"

namespace hm {    
    template<typename T>
    struct mat<4, 4, T> {
        typedef vec<4, T> col;
        
        col columns[4];
        
        mat() { mat(1.f); }
        
        mat(const T v) :
            columns {
                     col(v, 0.f, 0.f, 0.f),
                     col(0.f, v, 0.f, 0.f),
                     col(0.f, 0.f, v, 0.f),
                     col(0.f, 0.f, 0.f, v)
        }
        {};
        
        mat(const T x0, const T y0, const T z0, const T w0,
            const T x1, const T y1, const T z1, const T w1,
            const T x2, const T y2, const T z2, const T w2,
            const T x3, const T y3, const T z3, const T w3) :
            columns {
                     col(x0, y0, z0, w0),
                     col(x1, y1, z1, w1),
                     col(x2, y2, z2, w2),
                     col(x3, y3, z3, w3)
        }
        {};
        
        mat(const col& v0, const col& v1, const col& v2, const col& v3) :
            columns {
                     (v0),
                     (v1),
                     (v2),
                     (v3)
        }
        {};
        
        mat(const T* ptr) :
            columns {
                     col(ptr[0], ptr[1], ptr[2], ptr[3]),
                     col(ptr[4], ptr[5], ptr[6], ptr[7]),
                     col(ptr[8], ptr[9], ptr[10], ptr[11]),
                     col(ptr[12], ptr[13], ptr[14], ptr[15]),
        }
        {};
        
        
        // accessors
        
        col const& operator[](const uint8_t index) const {
            if (index < 4)
                return columns[index];
            
            // Cannot access higher indices in this mat4 (size: 4, tried: index)
            assert(index < 4);
            return columns[0];
        };
        
        col& operator[](const uint8_t index) {
            if (index < 4)
                return columns[index];
            
            // Cannot access higher indices in this mat4 (size: 4, tried: index)
            assert(index < 4);
            return columns[0];
        };
        
        
        // operators
        
        mat operator*(const mat& matrix) const {
            const col ia0 = columns[0];
            const col ia1 = columns[1];
            const col ia2 = columns[2];
            const col ia3 = columns[3];
            
            const col ib0 = matrix[0];
            const col ib1 = matrix[1];
            const col ib2 = matrix[2];
            const col ib3 = matrix[3];
            
            mat result;
            result[0] = ia0 * ib0[0] + ia1 * ib0[1] + ia2 * ib0[2] + ia3 * ib0[3];
            result[1] = ia0 * ib1[0] + ia1 * ib1[1] + ia2 * ib1[2] + ia3 * ib1[3];
            result[2] = ia0 * ib2[0] + ia1 * ib2[1] + ia2 * ib2[2] + ia3 * ib2[3];
            result[3] = ia0 * ib3[0] + ia1 * ib3[1] + ia2 * ib3[2] + ia3 * ib3[3];
            return result;
        };
        
        template<typename T>
        vec<4, T> operator*(const vec<4, T>& vector) const {
            
            vec<4, T> result;
            result.x = vector.x * columns[0][0] + vector.y * columns[1][0] + vector.z * columns[2][0] + vector.w * columns[3][0];
            result.y = vector.x * columns[0][1] + vector.y * columns[1][1] + vector.z * columns[2][1] + vector.w * columns[3][1];
            result.z = vector.x * columns[0][2] + vector.y * columns[1][2] + vector.z * columns[2][2] + vector.w * columns[3][2];
            result.w = vector.x * columns[0][3] + vector.y * columns[1][3] + vector.z * columns[2][3] + vector.w * columns[3][3];
            return result;
            
            /*            
            vec<4, T> const Mov0(vector[0]);
            vec<4, T> const Mov1(vector[1]);
            vec<4, T> const Mul0 = columns[0] * Mov0;
            vec<4, T> const Mul1 = columns[1] * Mov1;
            vec<4, T> const Add0 = Mul0 + Mul1;
            vec<4, T> const Mov2(vector[2]);
            vec<4, T> const Mov3(vector[3]);
            vec<4, T> const Mul2 = columns[2] * Mov2;
            vec<4, T> const Mul3 = columns[3] * Mov3;
            vec<4, T> const Add1 = Mul2 + Mul3;
            vec<4, T> const Add2 = Add0 + Add1;
            return Add2;
            */
        };
    };
    
    typedef mat<4, 4, float> mat4f;
    typedef mat<4, 4, double> mat4d;
    typedef mat<4, 4, int32_t> mat4i;
    
    template<typename T>
    static inline mat<4, 4, T> translate(const mat<4, 4, T>& matrix, const vec<3, T>& position) {
        mat<4, 4, T> m = matrix;
        m[3] = matrix.columns[0] * position.x + matrix.columns[1] * position.y + matrix.columns[2] * position.z + matrix.columns[3];
        return m;
    };
    
    template<typename T>
    static inline mat<4, 4, T> translate(const mat<4, 4, T>& matrix, const vec<2, T>& position) {
        return translate(matrix, vec<3, T>(position.x, position.y, 0));
    };
    
    template<typename T>
    static inline mat<4, 4, T> scale(const mat<4, 4, T>& matrix, const vec<3, T>& factor) {
        mat<4, 4, T>  mr = matrix;
        mr[0] = mr[0] * factor.x;
        mr[1] = mr[1] * factor.y;
        mr[2] = mr[2] * factor.z;
        return mr;
    };
    
    template<typename T>
    static inline mat<4, 4, T> scale(const mat<4, 4, T>& matrix, const vec<2, T>& factor) {
        return scale(matrix, vec<3, T>(factor.x, factor.y, 1));
    };
    
    template<typename T>
    static mat<4, 4, T> rotate(const mat<4, 4, T>& matrix, const T angle, const vec<3, T>& axisVector) {
        const float c = (float) std::cos(to_radians(angle));
        const float s = (float) std::sin(to_radians(angle));
        const float c1 = 1.0f - c;
        
        mat<4, 4, T>  mr(0.0f);
        mr[3][3] = 1.0f;
        mr[0][0] = axisVector.x * axisVector.x * c1 + c;
        mr[0][1] = axisVector.x * axisVector.y * c1 + axisVector.z * s;
        mr[0][2] = axisVector.x * axisVector.z * c1 - axisVector.y * s;
        
        mr[1][0] = axisVector.x * axisVector.y * c1 - axisVector.z * s;
        mr[1][1] = axisVector.y * axisVector.y * c1 + c;
        mr[1][2] = axisVector.y * axisVector.z * c1 + axisVector.x * s;
        
        mr[2][0] = axisVector.x * axisVector.z * c1 + axisVector.y * s;
        mr[2][1] = axisVector.y * axisVector.z * c1 - axisVector.x * s;
        mr[2][2] = axisVector.z * axisVector.z * c1 + c;
        
        return matrix * mr;
    };
    
    template<typename T>
    static inline mat<4, 4, T> rotate(const mat<4, 4, T>& matrix, const vec<3, T>& vector) {        
        mat<4, 4, T> m = matrix;
        m = rotate(m, vector.x, hm::vec3f(1, 0, 0));
        m = rotate(m, vector.y, hm::vec3f(0, 1, 0)); // we need negative angle here because of rotation definition
        m = rotate(m, vector.z, hm::vec3f(0, 0, 1));
        return m;        
    };

    template<typename T>
    static mat<4, 4, T> inverse(mat<4, 4, T> const& m) {        
        T Coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        T Coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
        T Coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

        T Coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        T Coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
        T Coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

        T Coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        T Coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
        T Coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

        T Coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        T Coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
        T Coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

        T Coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        T Coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
        T Coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

        T Coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
        T Coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
        T Coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

        vec<4, T> Fac0(Coef00, Coef00, Coef02, Coef03);
        vec<4, T> Fac1(Coef04, Coef04, Coef06, Coef07);
        vec<4, T> Fac2(Coef08, Coef08, Coef10, Coef11);
        vec<4, T> Fac3(Coef12, Coef12, Coef14, Coef15);
        vec<4, T> Fac4(Coef16, Coef16, Coef18, Coef19);
        vec<4, T> Fac5(Coef20, Coef20, Coef22, Coef23);

        vec<4, T> Vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
        vec<4, T> Vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
        vec<4, T> Vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
        vec<4, T> Vec3(m[1][3], m[0][3], m[0][3], m[0][3]);

        vec<4, T> Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
        vec<4, T> Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
        vec<4, T> Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
        vec<4, T> Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);

        vec<4, T> SignA(+1, -1, +1, -1);
        vec<4, T> SignB(-1, +1, -1, +1);
        mat<4, 4, T> Inverse(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

        vec<4, T> Row0(Inverse[0][0], Inverse[1][0], Inverse[2][0], Inverse[3][0]);

        vec<4, T> Dot0(m[0] * Row0);
        T Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

        T OneOverDeterminant = static_cast<T>(1) / Dot1;

        return Inverse * OneOverDeterminant;
    };    
};

#endif
