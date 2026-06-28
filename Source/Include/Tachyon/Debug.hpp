#pragma once

#include <Common.hpp>
#include <exception>

/* some parts taken from lk and ppsspp (thanks :>) */

#define DebugInfo(...) Debug::Report("[\e[0;37mTachyon INFO\e[0m]: " __VA_ARGS__)
#define DebugWarn(...) Debug::Report("[\e[0;93mTachyon WARNING\e[0m]: " __VA_ARGS__)
#define DebugError(...) Debug::Report("[\e[0;91mTachyon ERROR\e[0m]: " __VA_ARGS__)

#define TachyonAssert(Condition) \
    do { \
        if (unlikely((Condition) == false)) { \
            DebugError("Tachyon assertion failed at (%s:%d): %s\n", __FILE__, __LINE__, #Condition); \
            std::abort(); \
        }  \
    } while(false)

#define TachyonAssertMsg(Condition, ...) \
    do { \
        if (unlikely((Condition) == false)) { \
            DebugError("Tachyon assertion failed at (%s:%d): %s\n", __FILE__, __LINE__, #Condition); \
            DebugError("Assertion message: " __VA_ARGS__); \
            std::abort(); \
        }  \
    } while(false)

#define TachyonUnimplemented(...) \
    do { DebugError("Tachyon Unimplemented: " __VA_ARGS__); std::abort(); } while(false)

#define TachyonAbort(...) \
    do { DebugError(__VA_ARGS__); std::abort(); } while(false)
    

namespace Debug {
    void Report(const char * Message, ...);
};
