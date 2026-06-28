#include <Tachyon/Assembler.hpp>
#include <Tachyon/Debug.hpp>

void Tachyon_AssemblerAMD64::Xor(const GpReg & Operand1, const GpReg & Operand2) {
    /* looks like shit, but it works */
    if (Operand1.Is8bit()) {
        TachyonAssertMsg(Operand2.Is8bit(), "Both operands must be 8-bit\n");
    } else if (Operand1.Is16bit()) {
        TachyonAssertMsg(Operand2.Is16bit(), "Both operands must be 16-bit\n");
        this->EmitOpsizePrefix();
    } else if (Operand1.Is32bit()) {
        TachyonAssertMsg(Operand2.Is32bit(), "Both operands must be 32-bit\n");
    } else {
        TachyonAssertMsg(Operand2.Is64bit(), "Both operands must be 64-bit\n");
        uint8_t REX = this->InitRegsREX(Operand1, Operand2);
        this->Write8(REX);
    }
    uint8_t ModRM = 0xC0;
    this->SetModRM_Access(ModRM, ModType::DIRECT_REGISTER);
    this->SetModRM_Register(ModRM, Operand1, true, false);
    this->SetModRM_Register(ModRM, Operand2, false, false);

    this->Write8(0x31);
    this->Write8(ModRM);
}