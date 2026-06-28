#include <Tachyon/Assembler.hpp>
#include <Tachyon/Debug.hpp>

void Tachyon_AssemblerAMD64::Pop(const GpReg & Register) {
    TachyonAssertMsg(Register.Is32bit() == false, "Cannot use 32-bit operand size on POP\n");
    TachyonAssertMsg(Register.Is8bit() == false, "Cannot use 8-bit operand size on POP\n");
    if (Register.Is16bit()) {
        this->EmitOpsizePrefix();
    }
    if (Register.RequiresREX()) {
        this->EmitREX(false, false, false, true);
    }
    this->Write8(0x58 + Register.AsRegID());
}