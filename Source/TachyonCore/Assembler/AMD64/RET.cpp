#include <Tachyon/Assembler.hpp>

void Tachyon_AssemblerAMD64::Ret(void) {
    this->Write8(0xC3);
}

void Tachyon_AssemblerAMD64::Ret(const uint16_t Bytes) {
    this->Write8(0xC2);
    this->Write16(Bytes);
}