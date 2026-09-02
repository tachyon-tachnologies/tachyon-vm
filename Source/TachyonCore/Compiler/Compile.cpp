#include <Scratch/Common.hpp>
#include <Tachyon/Compiler.hpp>
#include <Tachyon/Scheduler.hpp>
#include <Tachyon/LRA.hpp>
#include <Tachyon/TinyIR.hpp>

#include <Scratch/Looks.hpp>

#include <unordered_map>
#include <bit>

using namespace Scratch;

static constexpr size_t MAX_ABI_REGISTER = ABIRegisterOrder::R9;

/* NOTE: this is x86-specific for now */
static constexpr size_t GetParamRegisterBase(void) {
#if (defined(_WIN32) || defined(_WIN64))
    return static_cast<size_t>(ABIRegisterOrder::RCX);
#else
    return static_cast<size_t>(ABIRegisterOrder::RDI);
#endif
}

static constexpr uint64_t GetVariableDataPointer(ScratchVariable * const Variable) {
    return std::bit_cast<uint64_t>(&Variable->GetData());
}

static ScratchVariable * FindVariableFromDataBinds(const auto & Inputs, size_t InputIndex, const TinyIR::DataBindMap & DataBinds) {
    auto Result = DataBinds.find(reinterpret_cast<void *>(Inputs[InputIndex]));
    if (unlikely(Result == DataBinds.end())) {
        return nullptr;
    }
    return reinterpret_cast<ScratchVariable *>(Result->first);
}

/* NOTE: again, this is x86-specific. when other architectures are added, i'll change this up */
static inline void __hot ABIPassParameter(const size_t i, TinyIR::IRValue & Value, TachyonAssembler & Asm) {
    if (Value.StackSpilled == true || Value.AllocatorId > MAX_ABI_REGISTER) {
        // TODO: stack spilling
        return;
    }
    
    GpReg AllocatedRegister(Value.AllocatorId);
    const size_t ParamRegisterBase = GetParamRegisterBase();
    const size_t ExpectedParamReg = ParamRegisterBase + i;
    
    if (Value.AllocatorId != ExpectedParamReg) {
        GpReg ParamRegister(ExpectedParamReg);
        /* caller-saved */
        Asm.Push(ParamRegister);
        Asm.Mov(ParamRegister, Value.GetValue());

        Value.TempPushed = true;
    } else {
        Asm.Mov(AllocatedRegister, Value.GetValue());
        Value.TempPushed = false;
    }
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
            TinyIR::DataBindMap & DataBinds = IRGen.GetDataBinds();
            auto & Inputs = IRBlock.GetInputs();
            auto & VarInput = IRGen.GetVariable(Inputs[1]);

            ScratchVariable * Variable = FindVariableFromDataBinds(Inputs, 1, DataBinds);
            TachyonAssert(Variable != nullptr);
            
            GpReg Register(VarInput.AllocatorId);
            Asm.Mov(Register, GetVariableDataPointer(Variable));
            
            auto & ValueInput = IRGen.GetVariable(Inputs[0]);
            Asm.Mov(GpReg(ValueInput.AllocatorId), ValueInput.GetValue());
            Asm.Mov(Mem(Register), GpReg(ValueInput.AllocatorId));
            break;
        }
        case TinyIR::IROpcodeType::RCALL: {
            switch(IRBlock.GetRCallType()) {
                case TinyIR::IRRCallType::INVALID: {
                    DebugError("Invalid runtime call!!\n");
                    break;
                }
                case TinyIR::IRRCallType::LOOKS_SAY: {
                    /* 
                     * Windows and Linux share an issue where allocations are not done near the runtime 
                     * It's so easy to fix for Windows, but it is absolute HELL for Linux.
                     *
                     * /proc/self/maps parsing didn't help me at all.
                     *
                     * The Linux kernel devs seriously need to make a reimplementation of the mmap
                     * syscall that is similar to VirtualAlloc2 on Windows systems.
                     * */
                    Asm.CallFunction(reinterpret_cast<void *>(RuntimeSay));
                    // Asm.Push(GpReg(GpReg::RAX));
                    // Asm.Mov(GpReg(GpReg::RAX), reinterpret_cast<uint64_t>(RuntimeSay));
                    //
                    // Asm.IndirectRegisterCall(GpReg(GpReg::RAX));
                    //
                    // Asm.Pop(GpReg(GpReg::RAX));
                    break;
                }
                default: {
                    DebugError("Unhandled runtime call type!!\n");
                    break;
                }
            }
            break;
        }
        case TinyIR::IROpcodeType::CALL: {
            TachyonAssert(IRBlock.BlockLinks[0] != nullptr);
            
            if (unlikely(IRBlock.Procedure->JITData.CodeEntry == nullptr)) {
                /* uncompiled. compile it */
                IRBlock.Procedure->JITData = Tachyon::Compile(
                    *Tachyon::GetCurrentScript(),
                    IRBlock.BlockLinks[0]->GetKey()
                );
            }
            
            auto & Inputs = IRBlock.GetInputs();
            
            size_t ParamIndex = 0;
            for(auto InputNum : Inputs) {
                TinyIR::IRValue & Value = IRGen.GetVariable(InputNum);
                ABIPassParameter(ParamIndex++, Value, Asm);
            }
            
            Asm.CallFunction(std::bit_cast<void *>(IRBlock.Procedure->JITData.CodeEntry));
 
            /* TODO: make it in the ACTUAL correct order */
            ParamIndex = 0;
            const size_t ParamRegisterBase = GetParamRegisterBase();
            for(auto InputNum : Inputs) {
                TinyIR::IRValue & Value = IRGen.GetVariable(InputNum);
                if (Value.TempPushed) {
                    Asm.Pop(GpReg(ParamRegisterBase + ParamIndex));
                }
                ParamIndex++;
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

    LinearRegAllocator RegAlloc;
    RegAlloc.Allocate(Variables);

    for(const auto IRBlock : BlockStack) {
        CompileIRBlock(Asm, IRGen, IRBlock);
    }
    
    return Asm.Commit();
}
