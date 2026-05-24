#include <Lib/Hexdump.hpp>
#include <cstddef>
#include <cstdint>
#include <ctype.h>
#include <stdio.h>

void Debug::Hexdump(const void * Base, const size_t Length) {
	uintptr_t Address = (uintptr_t)Base;
	for (size_t count = 0; count < Length; count += 16) {
		printf("0x%08llx: ", Address);
		printf("%08x %08x %08x %08x |", *(const uint32_t*)(Address), *(const uint32_t*)(Address + 4), *(const uint32_t*)(Address + 8), *(const uint32_t*)(Address + 12));
		for (int i = 0; i < 16; i++) {
			char c = *(const char*)(Address + i);
			if (isalpha(c)) {
				printf("%c", c);
			} else {
				printf(".");
			}
		}
		printf("|\n");
		Address += 16;
	}
}