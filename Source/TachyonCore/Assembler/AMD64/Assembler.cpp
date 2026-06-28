#include <Tachyon/Assembler.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/Blocks.hpp>
#include <Common.hpp>
#include <Lib/NanBox.hpp>

using namespace NanBox;

/*
    Commonly used functions
*/

void __hot Tachyon_AssemblerAMD64::EmitMainPrologue(Tachyon_JITState & State) {
    /*
        little stack representation after CALL :)

        [ RIP ] - 8 bytes
        [ R15 ] - 8 bytes

        in total, 16 bytes on the new stack
        16 byte alignment is needed for simd stuffies
    */
    this->Push(GpReg(GpReg::R15));
    this->Mov(GpReg::R15, reinterpret_cast<uint64_t>(&State));
}

void __hot Tachyon_AssemblerAMD64::EmitMainEpilogue(void) {
    /* restore r15 */
    this->Pop(GpReg(GpReg::R15));
    /* return code */
    this->Mov(GpReg::RAX, static_cast<uint64_t>(Scratch::ScratchStatus::SCRATCH_END));
    this->Xor(GpReg(GpReg::RDX), GpReg(GpReg::RDX));
    this->Ret();
}

/*
    REX
*/

void Tachyon_AssemblerAMD64::SetREX_Opsize(uint8_t & REX, const bool Opsize64) {
    REX &= ~0x8; // clear opsize extension flag
    REX |= Opsize64 << 3;
}

void Tachyon_AssemblerAMD64::SetREX_RegExtension(uint8_t & REX, const bool R) {
    REX &= ~0x4; // clear reg extension flag
    REX |= R << 2;
}

void Tachyon_AssemblerAMD64::SetREX_SIBExtension(uint8_t & REX, const bool X) {
    REX &= ~0x2; // clear index extension flag
    REX |= X << 1;
}

void Tachyon_AssemblerAMD64::SetREX_BaseExtension(uint8_t & REX, const bool B) {
    REX &= ~0x1; // clear base extension flag
    REX |= B;
}

// faster if REX is predetermined
void Tachyon_AssemblerAMD64::EmitREX(const bool Opsize64, const bool R, const bool X, const bool B) {
    uint8_t REX = (0x40 | (Opsize64 << 3) | (R << 2) | (X << 1) | B);
    this->Write8(REX);
}

uint8_t Tachyon_AssemblerAMD64::InitRegsREX(const GpReg & RM, const GpReg & Reg) {
    uint8_t REX = 0x40;
    this->SetREX_Opsize(REX, RM.Is64bit());
    this->SetREX_BaseExtension(REX, RM.RequiresREX());
    this->SetREX_RegExtension(REX, Reg.RequiresREX());
    return REX;
}

/*
    Prefixes
*/

// op-size prefix (32 -> 16)
void Tachyon_AssemblerAMD64::EmitOpsizePrefix(void) {
    this->Write8(0x66);
}

// addr-size prefix (64 -> 32)
void Tachyon_AssemblerAMD64::EmitAddrsizePrefix(void) {
    this->Write8(0x67);
}

/*
    ModRM
*/

void Tachyon_AssemblerAMD64::SetREG(uint8_t & ModRM, const uint8_t Reg) {
    ModRM &= ~0x38; // REG bits
    ModRM |= (Reg & 0b111) << 3;
}

void Tachyon_AssemblerAMD64::SetRM(uint8_t & ModRM, const uint8_t RM) {
    ModRM &= ~0x7; // RM bits
    ModRM |= (RM & 0b111);
}

void Tachyon_AssemblerAMD64::SetModRM_Access(uint8_t & ModRM, const ModType Access) {
    ModRM &= ~0xC0; // clear mod bits first
    ModRM |= Access << 6;
}

void Tachyon_AssemblerAMD64::SetModRM_Register(uint8_t & ModRM, const GpReg & Register, const bool IsRM, const bool ShouldEmitREX) {
    switch(Register) {
        case GpReg::SPL...GpReg::DIL: {
            if (ShouldEmitREX == true) {
                this->EmitREX(0, !IsRM, 0, IsRM);
            }
            IsRM ? this->SetRM(ModRM, Register.AsRegID()) : this->SetREG(ModRM, Register.AsRegID());
            break;
        }
        case GpReg::R8L...GpReg::R15L:
        case GpReg::R8W...GpReg::R15W:
        case GpReg::R8D...GpReg::R15D: {
            if (ShouldEmitREX == true) {
                this->EmitREX(0, IsRM, 0, IsRM);
            }
            IsRM ? this->SetRM(ModRM, Register.AsRegID()) : this->SetREG(ModRM, Register.AsRegID());
            break;
        }
        case GpReg::R8...GpReg::R15: {
            if (ShouldEmitREX == true) {
                this->EmitREX(1, !IsRM, 0, IsRM);
            }
            IsRM ? this->SetRM(ModRM, Register.AsRegID()) : this->SetREG(ModRM, Register.AsRegID());
            break;
        }
        case GpReg::RAX...GpReg::RDI: {
            if (ShouldEmitREX == true) {
                this->EmitREX(1, 0, 0, 0);
            }
            IsRM ? this->SetRM(ModRM, Register.AsRegID()) : this->SetREG(ModRM, Register.AsRegID());
            break;
        }
        /* non REX */
        default: {
            IsRM ? this->SetRM(ModRM, Register.AsRegID()) : this->SetREG(ModRM, Register.AsRegID());
            break;
        }
    }
}