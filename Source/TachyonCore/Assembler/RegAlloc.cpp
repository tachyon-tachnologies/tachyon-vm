#include <Tachyon/Assembler.hpp>
#include <Tachyon/RegAlloc.hpp>

std::optional<GpReg> RegisterAllocator::AllocateScratch(void) {
    for(auto & Register : this->ScratchRegisters) {
        if (Register.second == false) {
            Register.second = true;
            return Register.first;
        }
    }
    return std::nullopt;
}

std::optional<GpReg> RegisterAllocator::AllocatePreserved(void) {
    for(auto & Register : this->PreservedRegisters) {
        if (Register.second == false) {
            Register.second = true;
            return Register.first;
        }
    }
    return std::nullopt;
}

void RegisterAllocator::FreeScratch(const GpReg::RegisterKind ScratchReg) {
    for (auto & Register : this->ScratchRegisters) {
        if (Register.first == ScratchReg) {
            Register.second = false;
            return;
        }
    }
}

void RegisterAllocator::FreePreserved(const GpReg::RegisterKind ScratchReg) {
    for (auto & Register : this->PreservedRegisters) {
        if (Register.first == ScratchReg) {
            Register.second = false;
            return;
        }
    }
}