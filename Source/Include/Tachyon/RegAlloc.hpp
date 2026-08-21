#pragma once

#include <array>
#include <optional>
#include <utility>

#include <Tachyon/Assembler.hpp>
#include <Lib/NanBox.hpp>

class RegisterAllocator {
    private:
        /* [0] = Register kind, [1] = Used */
        using RegisterState = std::pair<GpReg::RegisterKind, bool>;

        std::array<RegisterState, 9> ScratchRegisters {
            RegisterState { GpReg::RAX, false },
            RegisterState { GpReg::RDI, false },
            RegisterState { GpReg::RSI, false },
            RegisterState { GpReg::RDX, false },
            RegisterState { GpReg::RCX, false },
            RegisterState { GpReg::R8, false },
            RegisterState { GpReg::R9, false },
            RegisterState { GpReg::R10, false },
            RegisterState { GpReg::R11, false }
        };

        std::array<RegisterState, 4> PreservedRegisters {
            RegisterState { GpReg::RBX, false },
            RegisterState { GpReg::R12, false },
            RegisterState { GpReg::R13, false },
            RegisterState { GpReg::R14, false },
        };

        std::unordered_map<NanBox::BoxedValue, GpReg::RegisterKind> ValueMap;
    public:
        std::optional<GpReg> AllocatePreserved(void);
        std::optional<GpReg> AllocateScratch(void);

        void FreeScratch(const GpReg::RegisterKind ScratchReg);
        void FreePreserved(const GpReg::RegisterKind ScratchReg);
};