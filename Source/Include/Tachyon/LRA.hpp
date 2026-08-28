#pragma once

#include <array>
#include <vector>

#include <Tachyon/TinyIR.hpp>

#if (defined(_WIN32) || defined(_WIN64))
enum ABIRegisterOrder : uint8_t {
    RCX = 1,
    RDX = 2,
    R8  = 8,
    R9  = 9,
};
#else
enum ABIRegisterOrder : uint8_t {
    RDI = 7,
    RSI = 6,
    RDX = 2,
    RCX = 1,
    R8  = 8,
    R9  = 9,
};
#endif

/**
 * Implements the linear register allocator
 */
class LinearRegAllocator {
    private:
        struct RegisterState {
            size_t RegId;
            size_t LastUsed;
        };

        // values can have the same allocated register id at the same time 
#if (defined(_WIN32) || defined(_WIN64))
        std::array<RegisterState, 7> Registers = {
            RegisterState {0, 0},
            RegisterState {1, 0},
            RegisterState {2, 0},
            RegisterState {8, 0},
            RegisterState {9, 0},
            RegisterState {10, 0},
            RegisterState {11, 0},
        };
#else
        std::array<RegisterState, 9> Registers = {
            RegisterState {0, 0},
            RegisterState {7, 0},
            RegisterState {6, 0},
            RegisterState {2, 0},
            RegisterState {1, 0},
            RegisterState {8, 0},
            RegisterState {9, 0},
            RegisterState {10, 0},
            RegisterState {11, 0}
        };
#endif
    public:
        void Allocate(std::vector<TinyIR::IRValue> & Values);
};