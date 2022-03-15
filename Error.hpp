#ifndef ERROR_HPP
#define ERROR_HPP

#include "Log.hpp"

#define ERR_ASSERTION_STR(title, msg) Log::format("%s: %s \n\tIn file: %s \n\tAt line: %u \n\tIn function: %s", title, msg, __FILE__, __LINE__, __PRETTY_FUNCTION__)

#ifndef ERR_NO_CHECKS
#define ASSERT(pred, msg)                                                                                                                                  \
    if (not(pred))                                                                                                                                         \
    {                                                                                                                                                      \
        Log::error(ERR_ASSERTION_STR("Assertion failed", msg)); \
        std::abort();                                                                                                                                      \
    }

#define SOFT_ASSERT(pred, msg)                                                                                                                                 \
    if (not(pred))                                                                                                                                             \
    {                                                                                                                                                          \
        Log::warn(ERR_ASSERTION_STR("Soft assertion failed", msg)); \
    }
#else
#define ASSERT(...)
#define SOFT_ASSERT(...)
#endif

#endif