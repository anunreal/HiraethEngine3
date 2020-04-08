#ifndef HE_CORE_H
#define HE_CORE_H

#include <string>
#include "heTypes.h"

// writes given message with the prefix into cout (thread safe) with the prefix before the message and a time stamp
extern HE_API void heLogCout(std::string const& message, std::string const& prefix);

#ifdef HE_ENABLE_LOGGING_ALL
#define HE_ENABLE_LOG_MSG
#define HE_ENABLE_DEBUG_MSG
#define HE_ENABLE_WARNING_MSG
#define HE_ENABLE_ERROR_MSG
#endif


#ifdef HE_ENABLE_LOG_MSG
#define HE_LOG(msg) heLogCout(msg, "[LOG  ]:")
#else
#define HE_LOG(msg)
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

#endif