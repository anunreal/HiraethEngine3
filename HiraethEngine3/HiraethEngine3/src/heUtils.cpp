#include "hepch.h"
#include "heUtils.h"

std::string SPACE_CHARS = "\t\n\v\f\r";

void heRandomCreate(HeRandom* random, uint32_t const seed) {
    if(seed > 0)
        random->gen = std::mt19937(seed);
    else {
        std::random_device device;
        random->gen = std::mt19937(device());    
    }
};

int32_t heRandomInt(HeRandom* random, int32_t const low, int32_t const high) {
    random->idis = std::uniform_int_distribution<>(low, high);
    return random->idis(random->gen);
};

float heRandomFloat(HeRandom* random, float const low, float const high) {
    random->fdis = std::uniform_real_distribution<float>(low, high);
    return random->fdis(random->gen);
};


void hePerlinNoiseCreate(HePerlinNoise* noise) {
    for(uint16_t x = 0; x < 512; ++x) {
        noise->p[x] = noise->permutation[x%256];
    }
};

// t must be between 0 and 1. Smooths the graph, meaning that values close to 0 and 1 will be smoothened, values
// close to 0.5 wont be changed much
double fade(double const t) {
    return t * t * t * (t * (t * 6 - 15) + 10);			// 6t^5 - 15t^4 + 10t^3
};

double lerp(double const a, double const b, double const x) {
    return a + x * (b - a);
};

double grad(int const hash, double const x, double const y, double const z) {
    int h = hash & 15; // Take the hashed value and take the first 4 bits of it (15 == 0b1111)
    double u = h < 8 /* 0b1000 */ ? x : y; // If the most significant bit (MSB) of the hash is 0 then set u = x.  Otherwise y.
		
    double v; // In Ken Perlin's original implementation this was another conditional operator (?:).  I expanded it for readability.
		
    if(h < 4) // If the first and second significant bits are 0 set v = y
        v = y;
    else if(h == 12 || h == 14) // If the first and second significant bits are 1 set v = x
        v = x;
    else // If the first and second significant bits are not equal (0/1, 1/0) set v = z
        v = z;
		
    return ((h&1) == 0 ? u : -u)+((h&2) == 0 ? v : -v); // Use the last 2 bits to decide if u and v are positive or negative.  Then return their addition.
};

double hePerlinNoise3D(HePerlinNoise* noise, hm::vec3d const& position) {
    int32_t xi = (int32_t) std::floor(position.x) & 255; // gets the index to the permutation vector, this loops the coordinates to always be in between 0 and 255
    int32_t yi = (int32_t) std::floor(position.y) & 255; // gets the index to the permutation vector, this loops the coordinates to always be in between 0 and 255
    int32_t zi = (int32_t) std::floor(position.z) & 255; // gets the index to the permutation vector, this loops the coordinates to always be in between 0 and 255

    double xf = position.x - std::floor(position.x);
    double yf = position.y - std::floor(position.y);
    double zf = position.z - std::floor(position.z);
    double u = fade(xf);
    double v = fade(yf);
    double w = fade(zf);

    int32_t aaa, aba, aab, abb, baa, bba, bab, bbb; // the 8 corners of the unit cube
    aaa = noise->p[noise->p[noise->p[ xi     ] +  yi     ] +  zi     ];
    aba = noise->p[noise->p[noise->p[ xi     ] + (yi + 1)] +  zi     ];
    aab = noise->p[noise->p[noise->p[ xi     ] +  yi     ] + (zi + 1)];
    abb = noise->p[noise->p[noise->p[ xi     ] + (yi + 1)] + (zi + 1)];
    baa = noise->p[noise->p[noise->p[(xi + 1)] +  yi     ] +  zi     ];
    bba = noise->p[noise->p[noise->p[(xi + 1)] + (yi + 1)] +  zi     ];
    bab = noise->p[noise->p[noise->p[(xi + 1)] +  yi     ] + (zi + 1)];
    bbb = noise->p[noise->p[noise->p[(xi + 1)] + (yi + 1)] + (zi + 1)];

    double x1, x2, y1, y2;
    x1 = lerp(grad(aaa, xf, yf,      zf), grad(baa, xf - 1., yf,      zf), u);
    x2 = lerp(grad(aba, xf, yf - 1., zf), grad(bba, xf - 1., yf - 1., zf), u);
    y1 = lerp(x1, x2, v);

    x1 = lerp(grad(aab, xf, yf,      zf - 1.), grad(bab, xf - 1., yf,      zf - 1.), u);
    x2 = lerp(grad(abb, xf, yf - 1., zf - 1.), grad(bbb, xf - 1., yf - 1., zf - 1.), u);
    y2 = lerp(x1, x2, v);
        
    return (lerp(y1, y2, w) + 1.) / 2.;
};


std::vector<std::string> heStringSplit(const std::string& string, const char delimn) {
    std::vector<std::string> result;
    std::string::const_iterator start = string.begin();
    std::string::const_iterator end = string.end();
    std::string::const_iterator next = std::find(start, end, delimn);
    
    while (next != end) {
        result.emplace_back(std::string(start, next));
        start = next + 1;
        
        while(start != end && start[0] == delimn)
            ++start;
        
        next = std::find(start, end, delimn);
        
    }
    
    if(start != end)
        result.emplace_back(std::string(start, next));
    return result;
};

std::string heStringReplaceAll(const std::string& input, const std::string& from, const std::string& to) {
    std::string copy = input;
    size_t start_pos = 0;
    while((start_pos = copy.find(from, start_pos)) != std::string::npos) {
        copy.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    
    return copy;
};

std::string heStringReplaceAll(const std::string& input, char const from, const char to) {
    std::string copy = input;
    size_t start_pos = 0;
    while((start_pos = copy.find(from, start_pos)) != std::string::npos) {
        //copy.replace(start_pos, 1, to);
        copy[start_pos] = to;
        start_pos += 1; // Handles case where 'to' is a substring of 'from'
    }
    
    return copy;
};

b8 heStringStartsWith(const std::string& base, const std::string& check) {
    return base.compare(0, check.size(), check.c_str()) == 0;
};

void heStringEatSpacesLeft(std::string& string) {
    string.erase(0, string.find_first_not_of(SPACE_CHARS));
};

void heStringEatSpacesRight(std::string& string) {
    string.erase(string.find_last_not_of(SPACE_CHARS) + 1);
};
