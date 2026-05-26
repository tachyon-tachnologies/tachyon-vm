#include <Tachyon/Debug.hpp>
#include <Lib/Hexdump.hpp>
#include <cstddef>
#include <cstdint>
#include <stdio.h>

void Debug::Hexdump(const void * Base, const size_t Length) {
	DebugInfo("----- hexdump output start -----\n");
	const uint8_t * Pointer = static_cast<const uint8_t *>(Base);
	for (size_t i = 0; i < Length; i++) {
		printf("%02x ", Pointer[i]);
		if ((i % 16) == 0 && i != 0) {
			putchar('\n');
		}
	}
	putchar('\n');
	DebugInfo("----- hexdump output end -----\n");
}