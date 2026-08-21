#include <Tachyon/Assembler.hpp>

/*
    REX
*/

void Tachyon_AssemblerAMD64::SetREX_Opsize(uint8_t & REX, const bool Opsize64) {
    REX &= ~0x8; // clear opsize extension flag
    REX |= Opsize64 << 3;
}

void Tachyon_AssemblerAMD64::SetREX_RegExtension(uint8_t & REX, const bool R) {
    REX &= ~0x4; // clear reg extension flag
    REX |= R << 2;
}

void Tachyon_AssemblerAMD64::SetREX_SIBExtension(uint8_t & REX, const bool X) {
    REX &= ~0x2; // clear index extension flag
    REX |= X << 1;
}

void Tachyon_AssemblerAMD64::SetREX_BaseExtension(uint8_t & REX, const bool B) {
    REX &= ~0x1; // clear base extension flag
    REX |= B;
}

// faster if REX is predetermined
void Tachyon_AssemblerAMD64::EmitREX(const bool Opsize64, const bool R, const bool X, const bool B) {
    uint8_t REX = (0x40 | (Opsize64 << 3) | (R << 2) | (X << 1) | B);
    this->Write8(REX);
}

uint8_t Tachyon_AssemblerAMD64::InitRegsREX(const GpReg & RM, const GpReg & Reg) {
    uint8_t REX = 0x40;
    this->SetREX_Opsize(REX, RM.Is64bit());
    this->SetREX_BaseExtension(REX, RM.RequiresREX());
    this->SetREX_RegExtension(REX, Reg.RequiresREX());
    return REX;
}