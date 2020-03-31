#ifndef HE_UTILS_H
#define HE_UTILS_H
#include "heTypes.h"
#include <vector>
#include <string>

// splits given string by delimn and returns the different parts of the string as vector
extern HE_API std::vector<std::string> heStringSplit(const std::string& string, const char delimn);
// replaces all occurences of from in input to to
extern inline HE_API std::string heStringReplaceAll(const std::string& input, const std::string& from, const std::string& to);
// returns true if check is the beginning of base
extern inline HE_API b8 heStringStartsWith(const std::string& base, const std::string& check);

#endif