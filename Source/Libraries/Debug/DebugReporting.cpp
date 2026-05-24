#include <Tachyon/Debug.hpp>
#include <cstdarg>
#include <cstdio>

void __hot Debug::Report(const char * Fmt, ...) {
	va_list args;
	va_start(args, Fmt);
	vprintf(Fmt, args);
	va_end(args);
}
