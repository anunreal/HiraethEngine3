#ifndef HE_UTILS_H
#define HE_UTILS_H

#include "heTypes.h"
#include "hm/hm.hpp"

struct HeRandom {
    std::mt19937 gen;
    std::uniform_real_distribution<float> fdis;
    std::uniform_int_distribution<>       idis;
    
    HeRandom() : gen(0) {};
};

struct HePerlinNoise {
    // Hash lookup table as defined by Ken Perlin.  This is a randomly arranged array of all numbers from 0-255 inclusive.
    uint8_t permutation[256] = { 151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,
                                 30,69,142,8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,
                                 62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,
                                 125,136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,
                                 83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,
                                 143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,
                                 196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,
                                 226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,
                                 47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,
                                 163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,
                                 79,113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,
                                 210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,
                                 31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,
                                 236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };

    // a shuffled permutation vector
    uint8_t p[512];
};

// sets up a random number generator with given seed. If seed is zero, a random seed will be used
extern HE_API void heRandomCreate(HeRandom* random, uint32_t const seed);
// returns a random int between low and high with that random
extern HE_API int32_t heRandomInt(HeRandom* random, int32_t const low = 0, int32_t const high = 10000);
// returns a random int between low and high with that random
extern HE_API float heRandomFloat(HeRandom* random, float const low = 0, float const high = 10000);

extern HE_API void hePerlinNoiseCreate(HePerlinNoise* noise);
extern HE_API double hePerlinNoise3D(HePerlinNoise* noise, hm::vec3d const& position);

// splits given string by delimn and returns the different parts of the string as vector
extern HE_API std::vector<std::string> heStringSplit(std::string const& string, char const delimn);
// replaces all occurences of from in input to to
extern HE_API std::string heStringReplaceAll(std::string const& input, std::string const& from, std::string const& to);
// replaces all occurences of from in input to to
extern HE_API std::string heStringReplaceAll(std::string const& input, char const from, char const to);
// returns true if check is the beginning of base
extern inline HE_API b8 heStringStartsWith(const std::string& base, const std::string& check);
// removes all whitespace from the left of the string
extern inline HE_API void heStringEatSpacesLeft(std::string& string);
// removes all whitespace from the right of the string
extern inline HE_API void heStringEatSpacesRight(std::string& string);


#endif
