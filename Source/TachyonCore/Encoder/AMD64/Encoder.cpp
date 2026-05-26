#include <Tachyon/Encoder.hpp>
#include <Tachyon/Debug.hpp>

/*
    REX
*/

void Tachyon_AMD64Encoder::SetREX_Opsize(uint8_t & REX, const bool Opsize64) {
    REX &= ~0x8; // clear opsize extension flag
    REX |= Opsize64 << 3;
}

void Tachyon_AMD64Encoder::SetREX_RegExtension(uint8_t & REX, const bool R) {
    REX &= ~0x4; // clear reg extension flag
    REX |= R << 2;
}

void Tachyon_AMD64Encoder::SetREX_SIBExtension(uint8_t & REX, const bool X) {
    REX &= ~0x2; // clear index extension flag
    REX |= X << 1;
}

void Tachyon_AMD64Encoder::SetREX_BaseExtension(uint8_t & REX, const bool B) {
    REX &= ~0x1; // clear base extension flag
    REX |= B;
}

// faster if REX is predetermined
void Tachyon_AMD64Encoder::EmitREX(const bool Opsize64, const bool R, const bool X, const bool B) {
    uint8_t REX = (0x40 | (Opsize64 << 3) | (R << 2) | (X << 1) | B);
    this->Write8(REX);
}

/*
    Prefixes
*/

// op-size prefix (32 -> 16)
void Tachyon_AMD64Encoder::EmitOpsizePrefix(void) {
    this->Write8(0x66);
}

// addr-size prefix (64 -> 32)
void Tachyon_AMD64Encoder::EmitAddrsizePrefix(void) {
    this->Write8(0x67);
}

/*
    ModRM
*/

void Tachyon_AMD64Encoder::SetREG(uint8_t & ModRM, const uint8_t Reg) {
    ModRM &= ~0x38; // REG bits
    ModRM |= (Reg & 0b111) << 3;
}

void Tachyon_AMD64Encoder::SetRM(uint8_t & ModRM, const uint8_t RM) {
    ModRM &= ~0x7; // RM bits
    ModRM |= (RM & 0b111);
}

void Tachyon_AMD64Encoder::SetModRM_Access(uint8_t & ModRM, const ModType Access) {
    ModRM &= ~0xC0; // clear mod bits first
    ModRM |= Access << 6;
}

void Tachyon_AMD64Encoder::SetModRM_Register(uint8_t & ModRM, const GpReg Register, const bool IsRM, const bool ShouldEmitREX) {
    switch(Register) {
        case GpReg::REG_SPL...GpReg::REG_DIL: {
            if (ShouldEmitREX == true) {
                this->EmitREX(0, !IsRM, 0, IsRM);
            }
            IsRM ? this->SetRM(ModRM, Register.AsRegID()) : this->SetREG(ModRM, Register.AsRegID());
            break;
        }
        case GpReg::REG_R8L...GpReg::REG_R15L:
        case GpReg::REG_R8W...GpReg::REG_R15W:
        case GpReg::REG_R8D...GpReg::REG_R15D: {
            if (ShouldEmitREX == true) {
                this->EmitREX(0, IsRM, 0, IsRM);
            }
            IsRM ? this->SetRM(ModRM, Register.AsRegID()) : this->SetREG(ModRM, Register.AsRegID());
            break;
        }
        case GpReg::REG_R8...GpReg::REG_R15: {
            if (ShouldEmitREX == true) {
                this->EmitREX(1, !IsRM, 0, IsRM);
            }
            IsRM ? this->SetRM(ModRM, Register.AsRegID()) : this->SetREG(ModRM, Register.AsRegID());
            break;
        }
        case GpReg::REG_RAX...GpReg::REG_RDI: {
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