#include <Scratch/Common.hpp>
#include <Tachyon/TinyIR.hpp>
#include <Tachyon/Compiler.hpp>
#include <Tachyon/Scheduler.hpp>

using namespace Scratch;

static void __hot CompileIRBlock(TachyonAssembler & Asm, const TinyIR::IROpcode & IRBlock) {
    switch(IRBlock.GetType()) {
        case TinyIR::IROpcodeType::START: {
            ScratchScript * Script = Tachyon::GetCurrentScript();
            TachyonAssertMsg(Script != nullptr, "Current script is null!\n");
            Asm.EmitMainPrologue(Script->JITState);
            break;
        }
        case TinyIR::IROpcodeType::ENTER: {
            ScratchScript * Script = Tachyon::GetCurrentScript();
            TachyonAssertMsg(Script != nullptr, "Current script is null!\n");
            Asm.EmitMainPrologue(Script->JITState);
            break;
        }
        case TinyIR::IROpcodeType::CALL: {
            /* the first block link = procedure definition always */
            TachyonAssert(IRBlock.BlockLinks[0] != nullptr);
            if (IRBlock.Procedure->JITData.CodeEntry == nullptr) {
                IRBlock.Procedure->JITData = Tachyon::Compile(*Tachyon::GetCurrentScript(), IRBlock.BlockLinks[0]->GetKey());
            }
            Asm.CallFunction(reinterpret_cast<void *>(IRBlock.Procedure->JITData.CodeEntry));
            break;
        }
        case TinyIR::IROpcodeType::END: {
            Asm.EmitMainEpilogue();
            break;
        };
        default: {
            DebugInfo("Unknown IR block!!\n");
            break;
        }
    }
}

OutputCodeInfo __hot Tachyon::Compile(ScratchScript & Script, std::string BlockId) {
    /*
        JIT pipeline:

        [ JSON Parse ]
        |
        [ IR gen ]
        |
        [ Machine code gen ]
        |
        [ Execution ]
    */
    ScratchBlock * Block = Script.Sprite->GetBlockFromId(BlockId);
    CompilerAssert(Block != nullptr, "Invalid block ID '%s'\n", BlockId.c_str());
    CompilerAssert(Block->GetParentKey().empty() == true, "Block '%s' is not a hat block!!\n", BlockId.c_str());

    TinyIR::IRGenerator IRGen;
    const TinyIR::IRStack & BlockStack = IRGen.GenerateIR(*Block);

    TachyonAssembler Asm;

    for(const auto IRBlock : BlockStack) {
        CompileIRBlock(Asm, IRBlock);
    }
    
    return Asm.Commit();
}