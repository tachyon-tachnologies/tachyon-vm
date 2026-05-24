#include <Tachyon/Encoder.hpp>
#include <cstdint>

// IMMEDIATE

void Tachyon_AMD64Encoder::Mov(const GpReg Dest, uint8_t Src) {
    uint8_t Opcode = 0b10110000;
    SetModRM_Register(Opcode, Dest, true);
    TachyonAssertMsg(Dest.Is8bit() == true, "Invalid combination of operands. Should be mov r8, imm8");
    this->Write8(Opcode);
    this->Write8(Src);
}

void Tachyon_AMD64Encoder::Mov(const GpReg Dest, uint16_t Src) {
    uint8_t Opcode = 0b10111000;
    this->EmitOpsizePrefix();
    SetModRM_Register(Opcode, Dest, true);
    TachyonAssertMsg(Dest.Is16bit() == true, "Invalid combination of operands. Should be mov r16, imm16");
    this->Write8(Opcode);
    this->Write16(Src);
}

void Tachyon_AMD64Encoder::Mov(const GpReg Dest, uint32_t Src) {
    uint8_t Opcode = 0b10111000;
    SetModRM_Register(Opcode, Dest, true);
    TachyonAssertMsg(Dest.Is32bit() == true, "Invalid combination of operands. Should be mov r32, imm32");
    this->Write8(Opcode);
    this->Write32(Src);
}

void Tachyon_AMD64Encoder::Mov(const GpReg Dest, uint64_t Src) {
    uint8_t Opcode = 0b10111000;
    SetModRM_Register(Opcode, Dest, true);
    TachyonAssertMsg(Dest.Is64bit() == true, "Invalid combination of operands. Should be mov r64, imm64");
    this->Write8(Opcode);
    this->Write64(Src);
}

// Memory access versions

// mov r/m, reg
// 8-bit, 16-bit, 32-bit, and 64-bit
void Tachyon_AMD64Encoder::Mov(const Mem Dest, const GpReg Src) {
    /* addrsize, opsize, and finally rex */
    if (Dest.IsRegister()) {
        const GpReg Register = Dest.GetRegister();
        TachyonAssertMsg(Dest.GetRegister().Is16bit() == false, "Invalid 64-bit address! Should be either a DWORD PTR or QWORD PTR!");
        if (Register.Is32bit()) {
            this->EmitAddrsizePrefix();
        }
    }
    if (Src.Is16bit()) {
        this->EmitOpsizePrefix();
    }
    else if (Src.Is64bit()) {
        this->EmitREX(true, false, false, false);
    }
    /* opcode */
    uint8_t TargetOpcode = Src.Is8bit() == true ? TargetOpcode = 0x88 : TargetOpcode = 0x89;
    /* modr/m */
    uint8_t ModRM;
    // could possibly be turnt into a seperate function
    this->SetModRM_Register(ModRM, Src, false, false);
    switch (Dest.GetType()) {
        case Mem::MEM_REG: {
            // there's levels to this shit
            break;
        }
        case Mem::MEM_REG_DISP: {
            GpRegDisp RegDisp = Dest.GetRegisterDisp();
            this->SetModRM_Register(ModRM, RegDisp.first, true, false);
            if (std::holds_alternative<int32_t>(RegDisp.second) == true) {
                this->SetModRM_Access(ModRM, ModType::DWORD_DISPLACEMENT);
            }
            else {
                this->SetModRM_Access(ModRM, ModType::BYTE_DISPLACEMENT);
            }
            this->Write8(TargetOpcode);
            this->Write8(ModRM);
            Mem Dummy(1, GpReg::REG_AL, RegDisp.first);
            this->Write8(Dummy.GetSIB());

            if (std::holds_alternative<int32_t>(RegDisp.second)) {
                uint32_t Disp32 = (uint32_t)std::get<int32_t>(RegDisp.second);
                this->Write32(Disp32);
            }
            else {
                uint32_t Disp8 = (uint8_t)std::get<int8_t>(RegDisp.second);
                this->Write8(Disp8);
            }
            break;
        }
        case Mem::MEM_SIB: {
            TachyonUnimplemented("SIB encoding\n");
            __builtin_unreachable();
        }
    }
}