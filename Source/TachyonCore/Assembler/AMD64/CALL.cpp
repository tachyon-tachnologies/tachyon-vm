#include "Tachyon/Debug.hpp"
#include <Tachyon/Assembler.hpp>

void Tachyon_AssemblerAMD64::RelCall(const int32_t Disp32) {
    this->Write8(0xE8);
    this->Write32(Disp32);
}

void Tachyon_AssemblerAMD64::RelCall(const int16_t Disp16) {
    this->Write8(0xE8);
    this->Write16(Disp16);
}

void Tachyon_AssemblerAMD64::CallFunction(const void * const FunctionPtr) {
    uint64_t FuncAddress = reinterpret_cast<uint64_t>(FunctionPtr);
    uint64_t CodeAddress = reinterpret_cast<uint64_t>(this->GetCodePointer());
    int32_t Displacement = static_cast<int32_t>(FuncAddress - (CodeAddress + 5));
    this->RelCall(Displacement);
}

void Tachyon_AssemblerAMD64::IndirectCall(const GpReg & Register) {
    TachyonAssertMsg(Register.Is16bit() == false || Register.Is8bit() == false, "Invalid address! Should be either a DWORD PTR, or a QWORD PTR!");
    if (Register.Is32bit()) {
        this->EmitAddrsizePrefix();
    }
    uint8_t ModRM = 0x10;
    this->SetModRM_Register(ModRM, Register, true);
    this->Write8(0xFF);
    this->Write8(ModRM);
}
