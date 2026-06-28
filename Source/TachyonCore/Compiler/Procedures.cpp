#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Procedures.hpp>
#include <Tachyon/Compiler.hpp>

using namespace Scratch;

void __hot Tachyon::CompileProcedure(ScratchProcedure & Procedure, ScratchScript & Script, std::string BlockId) {
    TachyonAssembler Assembler;
#if defined(__x86_64__)
    Assembler.Push(GpReg(GpReg::RBP));

    ScratchBlock * Block = Script.Sprite->GetBlockFromId(BlockId);
    CompilerAssert(Block != nullptr, "Invalid block ID '%s'\n", BlockId.c_str());
    
    while(Block != nullptr) {
        CompilerAssert(Block->CompileBlock(Assembler) == ScratchStatus::SCRATCH_NEXT, "Block '%s' failed to compile\n", Block->GetOpcode().c_str());
        Block = Block->NextBlock_Pointer;
    }

    Assembler.Pop(GpReg(GpReg::RBP));
    Assembler.Ret();
#endif
    Procedure.JITData = Assembler.MakeExecutable();
}