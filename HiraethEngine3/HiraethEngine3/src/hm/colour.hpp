#pragma once

namespace hm {
    
    struct colour {
        int r, g, b, a;
        
        colour() : r(0), g(0), b(0), a(0) {};
        colour(int v) : r(v), g(v), b(v), a(255) {};
        colour(int r, int g, int b) : r(r), g(g), b(b), a(255) {};
        colour(int r, int g, int b, int a) : r(r), g(g), b(b), a(a) {};
        
        float getR() const { return r / 255.f; };
        float getG() const { return g / 255.f; };
        float getB() const { return b / 255.f; };
        float getA() const { return a / 255.f; };
        
    };
    
};