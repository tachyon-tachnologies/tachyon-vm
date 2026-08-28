#include <Tachyon/Assembler.hpp>
#include <Tachyon/Debug.hpp>
#include <cstdint>

/*
    MOVE Immediate -> Register 
*/

void Tachyon_AssemblerAMD64::Mov(const GpReg & Dest, uint8_t Src) {
    TachyonAssertMsg(Dest.Is8bit() == true, "Invalid combination of operands. Should be mov r8, imm8\n");
    uint8_t Opcode = 0b10110000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write8(Src);
}

void Tachyon_AssemblerAMD64::Mov(const GpReg & Dest, uint16_t Src) {
    TachyonAssertMsg(Dest.Is16bit() == true, "Invalid combination of operands. Should be mov r16, imm16\n");
    uint8_t Opcode = 0b10111000;
    this->EmitOpsizePrefix();
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write16(Src);
}

void Tachyon_AssemblerAMD64::Mov(const GpReg & Dest, uint32_t Src) {
    TachyonAssertMsg(Dest.Is32bit() == true, "Invalid combination of operands. Should be mov r32, imm32\n");
    uint8_t Opcode = 0b10111000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write32(Src);
}

void Tachyon_AssemblerAMD64::Mov(const GpReg & Dest, uint64_t Src) {
    TachyonAssertMsg(Dest.Is64bit() == true, "Invalid combination of operands. Should be movabs r64, imm64\n");
    uint8_t Opcode = 0b10111000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write64(Src);
}

/*
    MOVE Register -> Register 
*/


/**
 * mov r/m, reg
 * Operand sizes: 8-bit, 16-bit, 32-bit, 64-bit
 *
 * @param Dest The register to write to
 * @param Src The value to write
 */
void Tachyon_AssemblerAMD64::Mov(const GpReg & Dest, const GpReg & Src) {
    /* looks like shit, but i will try to refine it later */
    uint8_t Opcode = 0x89; // default (not 8-bit)
    if (Dest.Is64bit()) {
        TachyonAssertMsg(Src.Is64bit(), "Expected both operands to be 64-bit!!");
        this->EmitREX(true, Src.RequiresREX(), false, Dest.RequiresREX());
    } else if (Dest.Is32bit()) {
        TachyonAssertMsg(Src.Is32bit(), "Expected both operands to be 32-bit!!");
    } else if (Dest.Is16bit()) {
        TachyonAssertMsg(Src.Is16bit(), "Expected both operands to be 16-bit!!");
        this->EmitOpsizePrefix();
    } else if (Dest.Is8bit()) {
        TachyonAssertMsg(Src.Is8bit(), "Expected both operands to be 8-bit!!");
        Opcode = 0x88;
    }
    uint8_t ModRM = ModType::DIRECT_REGISTER << 6;
    this->SetModRM_Register(ModRM, Dest, true, false);
    this->SetModRM_Register(ModRM, Src, false, false);
    
    this->Write8(Opcode);
    this->Write8(ModRM);
}

/*
    MOVE Memory -> Register 
*/

/**
 * mov [r/m], reg
 * Operand sizes: 8-bit, 16-bit, 32-bit, 64-bit
 *
 * @param Dest A place in memory
 * @param Src The value to write
 */
void Tachyon_AssemblerAMD64::Mov(const Mem & Dest, const GpReg & Src) {
    /* address size, operand size, and finally REX */
    if (Dest.IsRegister()) {
        const GpReg Register = Dest.GetRegister();
        TachyonAssertMsg(Register.Is16bit() == false && Register.Is8bit() == false, "Invalid address! Should be either a BYTE PTR, DWORD PTR, QWORD PTR!\n");
        if (Register.RequiresAddrsizePrefix() == true) {
            this->EmitAddrsizePrefix();
        }
    }
    // REX may or may not be used. we don't know for certain yet
    uint8_t REX = 0x40;
    if (Src.Is16bit()) {
        this->EmitOpsizePrefix();
    }
    this->SetREX_Opsize(REX, Src.Is64bit());
    this->SetREX_RegExtension(REX, Src.RequiresREX());
    /* opcode */
    uint8_t TargetOpcode = Src.Is8bit() == true ? TargetOpcode = 0x88 : TargetOpcode = 0x89;
    /* ModRM */
    uint8_t ModRM;
    // TODO: could possibly be turnt into a seperate function
    this->SetModRM_Register(ModRM, Src, false, false);
    switch (Dest.GetType()) {
        case Mem::MEM_REG: {
            GpReg Reg = Dest.GetRegister();
            this->SetREX_BaseExtension(REX, Reg.RequiresREX());
            // something was changed in the rex byte. use it
            if (REX != 0x40) {
                this->Write8(REX);
            }
            this->SetModRM_Register(ModRM, Reg, true, false);
            this->Write8(TargetOpcode);

            if (Reg.ShouldMemAccessThroughRM() == true) {
                /* [r/m] */
                this->SetModRM_Access(ModRM, ModType::NO_DISPLACEMENT);
                this->Write8(ModRM);
                return;
            }
            if (Reg.IsBP() == true) {
                /* [r/m + disp8] */
                this->SetModRM_Access(ModRM, ModType::BYTE_DISPLACEMENT);
                this->Write8(ModRM);
                this->Write8(0);
                return;
            }
            /* [sib] */
            this->SetModRM_Access(ModRM, ModType::NO_DISPLACEMENT);
            this->Write8(ModRM);
            Mem Dummy(1, GpReg::AL, Reg);
            this->Write8(Dummy.GetSIB());
            this->Write8(0);
            break;
        }
        case Mem::MEM_REG_DISP: {
            GpRegDisp RegDisp = Dest.GetRegisterDisp();
            this->SetREX_BaseExtension(REX, RegDisp.first.RequiresREX());
            // something was changed in the rex byte. use it
            if (REX != 0x40) {
                this->Write8(REX);
            }
            this->SetModRM_Register(ModRM, RegDisp.first, true, false);
            // should we use disp8 or disp32
            if (std::holds_alternative<int32_t>(RegDisp.second) == true) {
                this->SetModRM_Access(ModRM, ModType::DWORD_DISPLACEMENT);
            } else {
                this->SetModRM_Access(ModRM, ModType::BYTE_DISPLACEMENT);
            }
            this->Write8(TargetOpcode);
            this->Write8(ModRM);

            if (RegDisp.first.IsSP() == false) {
                /* [r/m + disp8/disp32] */
                if (auto DispPtr = std::get_if<int32_t>(&RegDisp.second)) {
                    uint32_t Disp32 = static_cast<uint32_t>(*DispPtr);
                    this->Write32(Disp32);
                    return;
                }
                uint8_t Disp8 = (uint8_t)std::get<int8_t>(RegDisp.second);
                this->Write8(Disp8);
                return;
            }
            /* [sib + disp8/disp32] */
            Mem Dummy(1, GpReg::AL, RegDisp.first);
            this->Write8(Dummy.GetSIB());

            if (auto DispPtr = std::get_if<int32_t>(&RegDisp.second)) {
                uint32_t Disp32 = static_cast<uint32_t>(*DispPtr);
                this->Write32(Disp32);
                return;
            }

            uint8_t Disp8 = (uint8_t)std::get<int8_t>(RegDisp.second);
            this->Write8(Disp8);
            break;
        }
        case Mem::MEM_SIB: {
            // TODO: this should be easy
            TachyonUnimplemented("SIB encoding\n\n");
            __builtin_unreachable();
            break;
        }
    }
}
