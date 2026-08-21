#include <Tachyon/Assembler.hpp>

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