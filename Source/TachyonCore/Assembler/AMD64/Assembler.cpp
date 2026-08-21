#include <Tachyon/Assembler.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/Blocks.hpp>
#include <Common.hpp>
#include <Lib/NanBox.hpp>

using namespace NanBox;

/*
    Commonly used functions
*/

void __hot Tachyon_AssemblerAMD64::EmitMainPrologue(Tachyon_JITState & State) {
    /*
        little stack representation after CALL :)

        [ RIP ] - 8 bytes
        [ R15 ] - 8 bytes

        in total, 16 bytes on the new stack
        16 byte alignment is needed for simd stuffies
    */
    this->Push(GpReg(GpReg::R15));
    this->Mov(GpReg::R15, reinterpret_cast<uint64_t>(&State));
}

void __hot Tachyon_AssemblerAMD64::EmitMainEpilogue(void) {
    /* return code */ 
    this->Mov(GpReg::RAX, static_cast<uint64_t>(Scratch::ScratchStatus::SCRATCH_END));
    /* restore r15 */
    this->Pop(GpReg(GpReg::R15));
    /* return code */
    this->Ret();
}

/*
    Prefixes
*/

// op-size prefix (32 -> 16)
void Tachyon_AssemblerAMD64::EmitOpsizePrefix(void) {
    this->Write8(0x66);
}

// addr-size prefix (64 -> 32)
void Tachyon_AssemblerAMD64::EmitAddrsizePrefix(void) {
    this->Write8(0x67);
}