#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Compiler.hpp>

using namespace Scratch;

/**
 * Compiles a Scratch script into native machine code.
 */
void __hot Tachyon::Compile(ScratchScript & Script, std::string BlockId) {
    TachyonCompiler Compiler;
    ScratchBlock * Block = Script.Sprite->GetBlockFromId(BlockId);
    CompilerAssert(Block != nullptr, "Invalid block ID '%s'\n", BlockId.c_str());
    /* nothing much for now */
    while(Block != nullptr) {
        CompilerAssert(Block->CompileBlock(Compiler) == ScratchStatus::SCRATCH_NEXT, "Block '%s' failed to compile\n", Block->GetOpcode().c_str());
        Block = Block->NextBlock_Pointer;
    }

    Compiler.EmitMainEpilogue();
    Script.JITState.CodeInfo = Compiler.Commit();
}