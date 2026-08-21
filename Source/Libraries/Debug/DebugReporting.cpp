#include <Tachyon/Debug.hpp>
#include <cstdarg>
#include <cstdio>

void __hot Debug::Report(const char * Message, ...) {
    va_list ap;
    va_start(ap, Message);
    vprintf(Message, ap);
    va_end(ap);
}