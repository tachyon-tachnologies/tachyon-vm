#pragma once

#include <cstdint>

namespace Scratch {
    /**
     * Contains status codes returned by opcode handlers.
     */
    enum class ScratchStatus : uint8_t {
        SCRATCH_END, // 0
        SCRATCH_NEXT, // 1
        SCRATCH_PAUSE, // 2
        SCRATCH_WAIT, // 3
        SCRATCH_WAIT_UNTIL // 4
    };
};