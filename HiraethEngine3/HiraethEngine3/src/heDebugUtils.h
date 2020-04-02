#ifndef HE_DEBUG_UTILS_H
#define HE_DEBUG_UTILS_H

#include "heTypes.h"
#include <string>

struct HeD3LightSource;
extern HE_API void he_to_string(const HeD3LightSource* ptr, std::string& output);

struct HeD3Instance;
extern HE_API void he_to_string(const HeD3Instance* ptr, std::string& output);

struct HeD3Camera;
extern HE_API void he_to_string(const HeD3Camera* ptr, std::string& output);

#endif //HE_DEBUG_UTILS_H
