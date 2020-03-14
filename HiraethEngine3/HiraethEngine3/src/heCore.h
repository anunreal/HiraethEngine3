#pragma once
#include <string>
#include "heTypes.h"

// writes given message with the prefix into cout (thread safe) with the prefix before the message and a time stamp
extern HE_API void hnLogCout(const std::string& message, const std::string& prefix);

#ifdef HE_ENABLE_LOGGING_ALL
#define HE_ENABLE_DEBUG_MSG
#define HE_ENABLE_WARNING_MSG
#define HE_ENABLE_ERROR_MSG
#endif


#ifdef HE_ENABLE_DEBUG_MSG
#define HE_DEBUG(msg) hnLogCout(msg, "[DEBUG]:")
#else
#define HE_DEBUG(msg)
#endif


#ifdef HE_ENABLE_WARNING_MSG
#define HE_WARNING(msg) hnLogCout(msg, "[WARN ]:")
#else
#define HE_WARNING(msg)
#endif


#ifdef HE_ENABLE_ERROR_MSG
#define HE_ERROR(msg) hnLogCout(msg, "[ERROR]:")
#else
#define HE_ERROR(msg)
#endif
