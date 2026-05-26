#include <Tachyon/Encoder.hpp>
#include <Tachyon/Debug.hpp>

void Tachyon_AMD64Encoder::Push(const GpReg Register) {
    TachyonAssertMsg(Register.Is32bit() == false, "Cannot use 32-bit operand size on PUSH");
    TachyonAssertMsg(Register.Is8bit() == false, "Cannot use 8-bit operand size on PUSH");
    if (Register.Is16bit()) {
        this->EmitOpsizePrefix();
    }
    if (Register.RequiresREX()) {
        this->EmitREX(false, false, false, true);
    }
    this->Write8(0x50 + Register.AsRegID());
}

void Tachyon_AMD64Encoder::Push(uint32_t Imm) {
    this->Write8(0x68);
    this->Write32(Imm);
}

void Tachyon_AMD64Encoder::Push(uint8_t Imm) {
    this->Write8(0x6A);
    this->Write8(Imm);
}