#pragma once

namespace hm {
    
    struct colour {
        uint8_t r, g, b, a; // rgba in range of [0:255]
        float i; // intensity of the colour [0:inf]
        
        colour() : r(0), g(0), b(0), a(0), i(1.f) {};
        colour(uint8_t const v) : r(v), g(v), b(v), a(255), i(1.f) {};
        colour(uint8_t const v, float const i) : r(v), g(v), b(v), a(255), i(i) {};
        colour(uint8_t const r, uint8_t const g, uint8_t const b) : r(r), g(g), b(b), a(255), i(1.f) {};
        colour(uint8_t const r, uint8_t const g, uint8_t const b, uint8_t const a) : r(r), g(g), b(b), a(a), i(1.f) {};
        colour(uint8_t const r, uint8_t const g, uint8_t const b, uint8_t const a, float const i)
            : r(r), g(g), b(b), a(a), i(i) {};

        
        // operators

        inline hm::colour operator*(float const v) const {
            return hm::colour((uint8_t) (v * r), (uint8_t) (v * g), (uint8_t) (v * b), a, v * i);
        };

        inline hm::colour operator+(hm::colour const& c) const {
            return hm::colour(c.r + r, c.g + g, c.b + b, c.a + a, c.i + i);
        };
    };
    
    static float getR(colour const* col) { return (col->r * col->i) / 255.f; };
    static float getG(colour const* col) { return (col->g * col->i) / 255.f; };
    static float getB(colour const* col) { return (col->b * col->i) / 255.f; };
    static float getA(colour const* col) { return col->a / 255.f; };
    
    static inline uint32_t encodeColour(colour const& col) {
        return col.r | (col.g * (1 << 8)) | (col.b * (1 << 16)) | (col.a * (1 << 24));
    };
    
    static inline colour decodeColour(uint32_t const code) {
        colour c;
        c.a = (0xFF000000 & code) >> 24;
        c.b = (0x00FF0000 & code) >> 16;
        c.g = (0x0000FF00 & code) >> 8;
        c.r = (0x000000FF & code);
        return c;
    };

    static inline hm::colour interpolateColour(hm::colour const& lhs, hm::colour const& rhs, float blend) {
        return lhs * blend + rhs * (1.f - blend);
    };
};
