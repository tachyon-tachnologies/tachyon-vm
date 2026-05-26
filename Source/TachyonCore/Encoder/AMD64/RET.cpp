#include <Tachyon/Encoder.hpp>

void Tachyon_AMD64Encoder::Ret(void) {
    this->Write8(0xC3);
}

void Tachyon_AMD64Encoder::Ret(const uint16_t Bytes) {
    this->Write8(0xC2);
    this->Write16(Bytes);
}