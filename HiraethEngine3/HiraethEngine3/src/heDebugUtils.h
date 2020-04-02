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

// tries to run a command. These commands are mainly for dynamic debugging (print debug info, set variables...).
// If this is no valid command, false is returned
extern HE_API b8 heCommandRun(const std::string& command);
// runs a thread that checks the console for input and then runs commands. This must be run in a seperate thread as
// it will block io. The function will run as long as running is true. If running is a nullptr, the function will
// run until the application closes
extern HE_API void heCommandThread(b8* running);

#endif //HE_DEBUG_UTILS_H
