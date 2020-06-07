#ifndef HE_UTILS_H
#define HE_UTILS_H
#include "heTypes.h"
#include <vector>
#include <string>
#include <random>

struct HeRandom {
    std::mt19937 gen;
    std::uniform_real_distribution<float> fdis;
    std::uniform_int_distribution<>       idis;
    
    HeRandom() : gen(0) {};
};

// sets up a random number generator with given seed. If seed is zero, a random seed will be used
extern HE_API void heRandomCreate(HeRandom* random, uint32_t const seed);
// returns a random int between low and high with that random
extern HE_API int32_t heRandomInt(HeRandom* random, int32_t const low = 0, int32_t const high = 10000);
// returns a random int between low and high with that random
extern HE_API float heRandomFloat(HeRandom* random, float const low = 0, float const high = 10000);

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
