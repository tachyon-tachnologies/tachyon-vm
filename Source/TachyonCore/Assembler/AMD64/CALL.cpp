#include <Tachyon/Debug.hpp>
#include <Tachyon/Assembler.hpp>
#include <cstdint>

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
    uint64_t Displacement64 = FuncAddress - (CodeAddress + 5);

    TachyonAssertMsg(Displacement64 <= UINT32_MAX, "JIT call target is outside the 32-bit displacement range!! Calculated value: 0x%016zx\n", Displacement64);
    
    int32_t Displacement = static_cast<int32_t>(Displacement64);
    this->RelCall(Displacement);
}

void Tachyon_AssemblerAMD64::IndirectMemoryCall(const GpReg & Register) {
    TachyonAssertMsg(Register.Is16bit() == false || Register.Is8bit() == false, "Invalid address! Should be either a DWORD PTR, or a QWORD PTR!");
    if (Register.Is32bit()) {
        this->EmitAddrsizePrefix();
    }
    uint8_t ModRM = 0b00010000;
    this->SetModRM_Register(ModRM, Register, true);
    this->Write8(0xFF);
    this->Write8(ModRM);
}

void Tachyon_AssemblerAMD64::IndirectRegisterCall(const GpReg & Register) {
    TachyonAssertMsg(Register.Is16bit() == false || Register.Is8bit() == false, "Invalid operand! Should be either a DWORD, or a QWORD!");
    if (Register.Is32bit()) {
        this->EmitAddrsizePrefix();
    }
    uint8_t ModRM = 0b11010000;
    this->SetModRM_Register(ModRM, Register, true);
    this->Write8(0xFF);
    this->Write8(ModRM);
}
