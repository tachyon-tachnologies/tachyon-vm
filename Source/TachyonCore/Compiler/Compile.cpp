#include <Scratch/Common.hpp>
#include <Tachyon/Compiler.hpp>
#include <Tachyon/Scheduler.hpp>
#include <Tachyon/LRA.hpp>

#include <Scratch/Looks.hpp>

#include <unordered_map>
#include <bit>

using namespace Scratch;

static inline void __hot ABIPassParameter(size_t i, TinyIR::IRValue & Value, TachyonAssembler & Asm) {
    if (Value.StackSpilled == false && Value.AllocatorId <= ABIRegisterOrder::R9) {
        GpReg AllocatedRegister(Value.AllocatorId);
#if (defined(_WIN32) || defined(_WIN64))
        GpReg ParamRegister(static_cast<size_t>(ABIRegisterOrder::RCX + i));
        if (Value.AllocatorId != ABIRegisterOrder::RCX + i) {
            Asm.Push(ParamRegister);
            Asm.Mov(ParamRegister, Value.GetValue());
            Value.TempPushed = true;
            return;
        }
#else
        GpReg ParamRegister(static_cast<size_t>(ABIRegisterOrder::RDI + i));
        if (Value.AllocatorId != ABIRegisterOrder::RDI + i) {
            Asm.Push(ParamRegister);
            Asm.Mov(ParamRegister, Value.GetValue());
            Value.TempPushed = true;
            return;
        }
#endif
        Asm.Mov(AllocatedRegister, Value.GetValue());
        Value.TempPushed = false;
        return;
    }
    /* spill to the stack */
    return;
}

static void __hot CompileIRBlock(TachyonAssembler & Asm, TinyIR::IRGenerator & IRGen, const TinyIR::IROpcode & IRBlock) {
    ScratchScript & Script = *Tachyon::GetCurrentScript();
    if (likely(IRBlock.Block != nullptr)) {
        Script.JITState.BlockMap.emplace(IRBlock.Block->GetKey(), Asm.GetCodePointer());
    }
    switch(IRBlock.GetType()) {
        case TinyIR::IROpcodeType::START: {
            ScratchScript * Script = Tachyon::GetCurrentScript();
            TachyonAssertMsg(Script != nullptr, "Current script is null!\n");
            Asm.EmitMainPrologue(Script->JITState);
            break;
        }
        case TinyIR::IROpcodeType::ENTER: {
            Asm.EmitFunctionPrologue();
            break;
        }
        case TinyIR::IROpcodeType::SET: {
            // could generate something
            auto & Inputs = IRBlock.GetInputs();
            auto & DataBinds = IRGen.GetDataBinds();

            auto & VarInput = IRGen.GetVariable(Inputs[1]);

            GpReg Register(VarInput.AllocatorId);

            ScratchVariable * Variable = nullptr;
            for(auto & Item : DataBinds) {
                if (Inputs[1] == Item.second.first) {
                    // ScratchVariable->Data pointer
                    Variable = reinterpret_cast<ScratchVariable *>(Item.first);
                    Asm.Mov(Register, std::bit_cast<uint64_t>(&Variable->GetData()));
                    break;
                }
            }
            TachyonAssert(Variable != nullptr);
            auto & ValueInput = IRGen.GetVariable(Inputs[0]);
            Asm.Mov(GpReg(ValueInput.AllocatorId), ValueInput.GetValue());
            Asm.Mov(Mem(Register), GpReg(ValueInput.AllocatorId));
            break;
        }
        case TinyIR::IROpcodeType::RCALL: {
            /* ambiguous call to runtime */
            switch(IRBlock.GetRCallType()) {
                case TinyIR::IRRCallType::INVALID: {
                    DebugError("Invalid runtime call!!\n");
                    break;
                }
                case TinyIR::IRRCallType::LOOKS_SAY: {
                    Asm.Push(GpReg(GpReg::RAX));
                    Asm.Mov(GpReg(GpReg::RAX), reinterpret_cast<uint64_t>(RuntimeSay));
                    Asm.IndirectRegisterCall(GpReg(GpReg::RAX));
                    // Asm.CallFunction(reinterpret_cast<void *>(RuntimeSay));
                    Asm.Pop(GpReg(GpReg::RAX));
                    break;
                }
                default: {
                    DebugError("Unhandled runtime call type\n");
                    break;
                }
            }
            break;
        }
        case TinyIR::IROpcodeType::CALL: {
            /* the first block link = procedure definition always */
            TachyonAssert(IRBlock.BlockLinks[0] != nullptr);
            if (IRBlock.Procedure->JITData.CodeEntry == nullptr) {
                IRBlock.Procedure->JITData = Tachyon::Compile(*Tachyon::GetCurrentScript(), IRBlock.BlockLinks[0]->GetKey());
            }
            /* TODO: handle parameters */
            // DebugInfo("Compiler: function uses %d input(s)\n", IRBlock.GetNumInputs());
            auto & Inputs = IRBlock.GetInputs();
            size_t i = 0;
            for(auto InputNum : Inputs) {
                TinyIR::IRValue & Value = IRGen.GetVariable(InputNum);
                // DebugInfo("allocator id: %d, spilled: %s, last used: %d-%d\n", Value.AllocatorId, Value.StackSpilled ? "true" : "false", Value.FirstUsed, Value.LastUsed);
                ABIPassParameter(i++, Value, Asm);
            }
            Asm.CallFunction(reinterpret_cast<void *>(IRBlock.Procedure->JITData.CodeEntry));
            /* this looks jank asf but it works */
            i = 0;
            for(auto InputNum : Inputs) {
                TinyIR::IRValue & Value = IRGen.GetVariable(InputNum);
                if (Value.TempPushed) {
#if (defined(_WIN32) || defined(_WIN64))
                    Asm.Pop(GpReg(ABIRegisterOrder::RCX + i));
#else
                    Asm.Pop(GpReg(ABIRegisterOrder::RDI + i)
#endif
                }
                i++;
            }
            break;
        }
        case TinyIR::IROpcodeType::LEAVE: {
            Asm.EmitFunctionEpilogue();
            break;
        }
        case TinyIR::IROpcodeType::END: {
            Asm.EmitMainEpilogue();
            break;
        };
        case TinyIR::IROpcodeType::NOP: {
            // do not generate anything.
            break;
        }
        default: {
            DebugInfo("Unknown IR block!!\n");
            break;
        }
    }
}

Tachyon::OutputCodeInfo __hot Tachyon::Compile(ScratchScript & Script, std::string BlockId) {
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
    auto & Variables = IRGen.GetVariables();

    // for(size_t i = 0; i < Variables.size(); i++) {
    //     auto & Value = Variables.at(i);
    //     DebugInfo("v%d: %d - %d\n", i, Value.FirstUsed, Value.LastUsed);
    // }

    LinearRegAllocator RegAlloc;
    RegAlloc.Allocate(Variables);

    for(const auto IRBlock : BlockStack) {
        CompileIRBlock(Asm, IRGen, IRBlock);
    }
    
    return Asm.Commit();
}