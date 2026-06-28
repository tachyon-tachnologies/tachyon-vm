#include <Tachyon/Assembler.hpp>

void Tachyon_AssemblerAMD64::Sub(const GpReg & Reg, uint8_t Imm) {
    if (Reg.Is16bit()) {
        this->EmitOpsizePrefix();
    }
    uint8_t ModRM = 0xC0;
    this->SetModRM_Access(ModRM, ModType::DIRECT_REGISTER);
    this->SetREG(ModRM, 5);
    this->SetModRM_Register(ModRM, Reg, true, true);

    this->Write8(0x83);
    this->Write8(ModRM);
    this->Write8(Imm);
}