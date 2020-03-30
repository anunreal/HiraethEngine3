#pragma once
#include <string>
#include "heTypes.h"

// used to print out debug information on the fly
struct HeDebugInfo {
    // a pointer to a stream to which to print the information (can be a file stream). If this is nullptr, std::cout
    // will be used as default
    std::ostream* stream = nullptr;
    // flags set. When a flag is set, the information will be printed the next time its accessed (i.e. for the light
    // the next time a level is rendered), then that flag will be removed.
    uint32_t flags = 0;
};

// writes given message with the prefix into cout (thread safe) with the prefix before the message and a time stamp
extern HE_API void heLogCout(const std::string& message, const std::string& prefix);


// requests given debug information. This will set the given flag.
extern HE_API inline void heDebugRequestInfo(const HeDebugInfoFlags flags);
// returns true if the given flag was requested before (since the last check). This will remove the flag
extern HE_API inline bool heDebugIsInfoRequested(const HeDebugInfoFlags flag);
// sets the output stream for debug information (heDebugPrint)
extern HE_API inline void heDebugSetOutput(std::ostream* stream);
// prints given debug information to the stream set with heDebugSetOutput
extern HE_API void heDebugPrint(const std::string& message);

// tries to run a command. These commands are mainly for dynamic debugging (print debug info, set variables...).
// If this is no valid command, false is returned
extern HE_API bool heCommandRun(const std::string& command);

#ifdef HE_ENABLE_LOGGING_ALL
#define HE_ENABLE_DEBUG_MSG
#define HE_ENABLE_WARNING_MSG
#define HE_ENABLE_ERROR_MSG
#endif


#ifdef HE_ENABLE_DEBUG_MSG
#define HE_DEBUG(msg) heLogCout(msg, "[DEBUG]:")
#else
#define HE_DEBUG(msg)
#endif


#ifdef HE_ENABLE_WARNING_MSG
#define HE_WARNING(msg) heLogCout(msg, "[WARN ]:")
#else
#define HE_WARNING(msg)
#endif


#ifdef HE_ENABLE_ERROR_MSG
#define HE_ERROR(msg) heLogCout(msg, "[ERROR]:")
#else
#define HE_ERROR(msg)
#endif
