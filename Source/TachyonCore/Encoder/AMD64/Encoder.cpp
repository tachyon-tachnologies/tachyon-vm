#include <Tachyon/Encoder.hpp>

inline void Tachyon_AMD64Encoder::EmitREX(const bool Opsize64, const bool R, const bool X, const bool B) {
    uint8_t REX = (0x40 | (Opsize64 << 3) | (R << 2) | (X << 1) | B);
    this->Write8(REX);
}

// op-size prefix (32 -> 16)
inline void Tachyon_AMD64Encoder::EmitOpsizePrefix(void) {
    this->Write8(0x66);
}

// addr-size prefix (64 -> 32)
inline void Tachyon_AMD64Encoder::EmitAddrsizePrefix(void) {
    this->Write8(0x67);
}

inline void Tachyon_AMD64Encoder::SetREG(uint8_t & ModRM, const uint8_t Reg) {
    ModRM &= ~0x38; // REG bits
    ModRM |= (Reg & 0b111) << 3;
}

inline void Tachyon_AMD64Encoder::SetRM(uint8_t & ModRM, const uint8_t RM) {
    ModRM &= ~0x7; // RM bits
    ModRM |= (RM & 0b111);
}

inline void Tachyon_AMD64Encoder::SetModRM_Access(uint8_t & ModRM, const ModType Access) {
    ModRM |= Access << 6;
}

inline void Tachyon_AMD64Encoder::SetModRM_Register(uint8_t & ModRM, GpReg Register, const bool IsRM, const bool ShouldEmitREX = true) {
    switch(Register) {
        case GpReg::REG_SPL...GpReg::REG_DIL: {
            if (ShouldEmitREX == true) {
                this->EmitREX(0, !IsRM, 0, IsRM);
            }
            ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
            break;
        }
        case GpReg::REG_R8L...GpReg::REG_R15L:
        case GpReg::REG_R8W...GpReg::REG_R15W:
        case GpReg::REG_R8D...GpReg::REG_R15D: {
            if (ShouldEmitREX == true) {
                this->EmitREX(0, !IsRM, 0, IsRM);
            }
            ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
            break;
        }
        case GpReg::REG_R8...GpReg::REG_R15: {
            if (ShouldEmitREX == true) {
                this->EmitREX(1, !IsRM, 0, IsRM);
            }
            ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
            break;
        }
        case GpReg::REG_RAX...GpReg::REG_RDI: {
            if (ShouldEmitREX == true) {
                this->EmitREX(1, 0, 0, 0);
            }
            ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
            break;
        }
        /* non REX */
        default: {
            ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
            break;
        }
    }
}