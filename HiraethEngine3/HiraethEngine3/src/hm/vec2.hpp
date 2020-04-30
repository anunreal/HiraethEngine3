#pragma once
#include "setup.hpp"

namespace hm {
    
    template<typename T>
        struct vec2 {
        T x, y;
        
        vec2() : x(0), y(0) {};
        vec2(T const v) : x(v), y(v) {};
        vec2(T const x, T const y) : x(x), y(y) {};
        vec2(vec2 const& v) : x(v.x), y(v.y) {};
        template<typename T1>
            vec2(vec2<T1> const& v) : x((T)v.x), y((T)v.y) {};
        
        // operators
        
        inline vec2 operator*(vec2 const& v) const {
            return vec2(x * v.x, y * v.y);
        };
        
		inline vec2 operator*(T const v) const {
			return vec2(x * v, y * v);
		};

		inline vec2 operator+(vec2 const& v) const {
            return vec2(x + v.x, y + v.y);
        };
        
        inline vec2 operator-(vec2 const& v) const {
            return vec2(x - v.x, y - v.y);
        };
        
        inline vec2 operator/(vec2 const& v) const {
            return vec2(x / v.x, y / v.y);
        };
        
        inline vec2 operator/(double const v) const {
            return vec2((T) (x / v), (T) (y / v));
        };
        
        inline vec2& operator*=(double const v) {
            this->x *= v;
            this->y *= v;
            return *this;
        };
        
		inline vec2& operator/=(vec2 const& v) {
			this->x /= v.x;
			this->y /= v.y;
			return *this;
		};

		inline vec2& operator+=(vec2 const& v) {
            this->x += v.x;
            this->y += v.y;
            return *this;
        };
        
        inline vec2& operator-=(vec2 const& v) {
            this->x -= v.x;
            this->y -= v.y;
            return *this;
        };
        
        inline bool operator==(vec2 const& v) const {
            return this->x == v.x && this->y == v.y;
        };
        
        inline bool operator!=(vec2 const& v) const {
            return !operator==(v);
        };
    };
    
    typedef vec2<float> vec2f;
    typedef vec2<double> vec2d;
    typedef vec2<int32_t> vec2i;
    
    // rotates vec around pivotPoint with given degrees
    template<typename T>
	static vec2<T> rotate(vec2<T> const& vec, vec2<T> const& pivotPoint, float const degrees) {
        vec2<T> relative = vec - pivotPoint;
        float c = (float) std::cos(to_radians(degrees));
        float s = (float) std::sin(to_radians(degrees));
        float xnew = relative.x * c - relative.y * s;
        float ynew = relative.x * s + relative.y * c;
        return pivotPoint + vec2<T>((T) xnew, (T) ynew);
    };
    
    
    // decodes a vec2 from a int32_t (which should be previosuly recieved by encoding a vec2). Note that we can return
    // any vec2 type here, but the x and y coordinates will always be rounded (integers), because we cant store
    // floating point numbers in the int32_t
    template<typename T>
	static inline vec2<T> decodeVec2(int32_t const val) {
        vec2<T> v;
        v.x = (T) (val & 0x0000ffff);
        v.y = (T) ((val & 0xffff0000) >> 16);
        return v;
    };
    
    // Encodes an integer vec2 to an int32_t. Only int vec2 can be encoded because floats / double have commas (duh),
    // so we'd have to calculate and store a precision, which would just be dumb
    static inline int32_t encodeVec2(vec2i const& vec) {
        return vec.x | (vec.y * (1 << 16));
    };
    
    // no idea what were doing here but its working
    template<typename T>
	static inline T isLeft(vec2<T> const& p0, vec2<T> const& p1, vec2<T> const& p2) {
    	return ((p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y));
    };
    
    // returns true if point is inside the rectangle defined by r0-r3. These points may be defined in any space and can
    // be rotated, however they must be defined in clockwise order
    template<typename T>
	static bool pointInRectangle(vec2<T> const& r0, vec2<T> const& r1, vec2<T> const& r2, vec2<T> const& r3, vec2<T> const& point) {
        return isLeft(r0, r1, point) > 0 &&
            isLeft(r1, r2, point) > 0 &&
            isLeft(r2, r3, point) > 0 &&
            isLeft(r3, r0, point) > 0;
    };
    
    template<typename T>
	static T length(vec2<T> const& vec) {
        return std::sqrt(vec.x * vec.x + vec.y * vec.y);
    };
    
    template<typename T>
	static vec2<T> normalize(vec2<T> const& vec) {    
        T l = length(vec);
        return vec2<T>(vec.x / l, vec.y / l);
    };
    
};
