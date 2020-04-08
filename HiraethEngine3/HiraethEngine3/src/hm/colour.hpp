#pragma once

namespace hm {
    
    struct colour {
        uint8_t r, g, b, a; // rgba in range of [0:255]
        float i; // intensity of the colour [0:inf]
        
        colour() : r(0), g(0), b(0), a(0), i(1.f) {};
        colour(const uint8_t v) : r(v), g(v), b(v), a(255), i(1.f) {};
        colour(const uint8_t v, const float i) : r(v), g(v), b(v), a(255), i(i) {};
        colour(const uint8_t r, const uint8_t g, const uint8_t b) : r(r), g(g), b(b), a(255), i(1.f) {};
        colour(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) : r(r), g(g), b(b), a(a), i(1.f) {};
        //colour(const uint8_t r, const uint8_t g, const uint8_t b, const float i) : r(r), g(g), b(b), a(255), i(i) {};
        colour(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a, float i)
            : r(r), g(g), b(b), a(a), i(i) {};
    };
    
    static float getR(const colour* col) { return (col->r * col->i) / 255.f; };
    static float getG(const colour* col) { return (col->g * col->i) / 255.f; };
    static float getB(const colour* col) { return (col->b * col->i) / 255.f; };
    static float getA(const colour* col) { return col->a / 255.f; };
    
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
    
};