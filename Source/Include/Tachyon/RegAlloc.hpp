#pragma once

#include <cstdint>
#include <array>
#include <Tachyon/Assembler.hpp>

class RegAllocator {
    private:
#if defined(__x86_64__)
        std::array<uint8_t, 14> RegisterStack;
#endif
    public:
        int Allocate() {
            
        }
};