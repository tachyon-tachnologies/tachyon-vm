#include <Tachyon/Assembler.hpp>
#include <Tachyon/Debug.hpp>

void Tachyon_AssemblerAMD64::Push(const GpReg & Register) {
    TachyonAssertMsg(Register.Is32bit() == false, "Cannot use 32-bit operand size on PUSH\n");
    TachyonAssertMsg(Register.Is8bit() == false, "Cannot use 8-bit operand size on PUSH\n");
    if (Register.Is16bit()) {
        this->EmitOpsizePrefix();
    }
    if (Register.RequiresREX()) {
        this->EmitREX(false, false, false, true);
    }
    this->Write8(0x50 + Register.AsRegID());
}

void Tachyon_AssemblerAMD64::Push(uint32_t Imm) {
    this->Write8(0x68);
    this->Write32(Imm);
}

void Tachyon_AssemblerAMD64::Push(uint8_t Imm) {
    this->Write8(0x6A);
    this->Write8(Imm);
}