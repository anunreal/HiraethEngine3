#ifndef HE_UTILS_H
#define HE_UTILS_H
#include "heTypes.h"
#include <vector>
#include <string>

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
