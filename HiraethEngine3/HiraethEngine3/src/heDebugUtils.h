#ifndef HE_DEBUG_UTILS_H
#define HE_DEBUG_UTILS_H

#include "heTypes.h"
#include <string>

// used to print out debug information on the fly
struct HeDebugInfo {
    // a pointer to a stream to which to print the information (can be a file stream). If this is nullptr, std::cout
    // will be used as default
    std::ostream* stream = nullptr;
    // flags set. When a flag is set, the information will be printed the next time its accessed (i.e. for the light
    // the next time a level is rendered), then that flag will be removed.
    uint32_t flags = 0;
};

// returns true if the given flag was requested before (since the last check). This will remove the flag
extern HE_API inline b8 heDebugIsInfoRequested(const HeDebugInfoFlags flag);
// requests given debug information. This will set the given flag.
extern HE_API inline void heDebugRequestInfo(const HeDebugInfoFlags flags);
// sets the output stream for debug information (heDebugPrint)
extern HE_API inline void heDebugSetOutput(std::ostream* stream);
// prints given debug information to the stream set with heDebugSetOutput
extern HE_API void heDebugPrint(const std::string& message);


// -- enums -- (returning string because these are simple)

extern HE_API std::string he_to_string(HeLightSourceType const& type);
extern HE_API std::string he_to_string(HePhysicsShape const& type);

// -- structs -- (a big string, avoid copying with reference)

struct HeD3LightSource;
extern HE_API void he_to_string(HeD3LightSource const* ptr, std::string& output, std::string const& prefix = "");

struct HeD3Instance;
extern HE_API void he_to_string(HeD3Instance const* ptr, std::string& output, std::string const& prefix = "");

struct HeD3Camera;
extern HE_API void he_to_string(HeD3Camera const* ptr, std::string& output, std::string const& prefix = "");

struct HePhysicsComponent;
extern HE_API void he_to_string(HePhysicsComponent const* ptr, std::string& output, std::string const& prefix = "");

// tries to run a command. These commands are mainly for dynamic debugging (print debug info, set variables...).
// If this is no valid command, false is returned
extern HE_API b8 heCommandRun(std::string const& command);
// runs a thread that checks the console for input and then runs commands. This must be run in a seperate thread as
// it will block io. The function will run as long as running is true. If running is a nullptr, the function will
// run until the application closes
extern HE_API void heCommandThread(b8* running);



#endif //HE_DEBUG_UTILS_H
