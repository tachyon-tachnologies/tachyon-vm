#include <unordered_map>
#include <bit>

#include <Tachyon/Debug.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Assembler.hpp>
#include <Tachyon/Compiler.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/Data.hpp>
#include <Lib/NanBox.hpp>

using namespace NanBox;
using namespace Scratch;

void ScratchSprite::ResolveProcedureDefinitions(void) {
    for(auto & Item : this->ProcedureDefinitions) {
        auto & ProcDef = Item.second;
        const ScratchInput Input = ProcDef->GetInput(0);

        TachyonAssert(Input.IsType(InputType::ProcedureDefinition));

        struct ScratchProcedure Procedure;
        Procedure.DefinitionKey = ProcDef->GetKey();
        Procedure.PrototypeKey = std::get<std::string>(Input.Input);

        ScratchBlock * Prototype = this->GetBlockFromId(Procedure.PrototypeKey);
        const ScratchMutation & Mutation = Prototype->GetMutation();

        Procedure.UseWarp = Mutation.UseWarp;
        Procedure.ProcCode = Mutation.ProcCode;
        Procedure.ParametersNames = Mutation.ParametersNames;
        Procedure.ParametersKeys = Mutation.ParametersKeys;

        this->Procedures.emplace_back(Procedure);
    }
}

/**
 * Interpreter implementation for 'procedures_call'
 */
static ScratchStatus __hot i_ProceduresCall(ScratchBlock & Block) {
    ScratchMutation & Mutation = Block.GetMutation();
    ScratchSprite & Owner = Block.GetOwnerSprite();
    for (auto & Procedure : Owner.Procedures) {
        if (Procedure.ProcCode == Mutation.ProcCode) {
            ScratchScript * CurrentScript = Tachyon::GetCurrentScript();
            TachyonAssert(CurrentScript != nullptr);
            if (Tachyon::GetConfigVM() & TACHYON_CFG_PBLOCK) {
                if (Tachyon::Pseudo::IsPseudo(Procedure.ProcCode) == true) {
                    /* act as if nothing ever happened */
                    CurrentScript->CurrentBlockId = Block.GetNextKey();
                    return Tachyon::Pseudo::Execute(Procedure.ProcCode, Block);
                }
            }
            ScratchBlock * ProcBlock = Owner.GetBlockFromId(Procedure.DefinitionKey);
            TachyonAssert(ProcBlock != nullptr);

            /* bind parameters */
            size_t TotalParams = Procedure.ParametersNames.size();
            ProcedureBindings ParamBindings;
            ParamBindings.reserve(TotalParams);

            for(size_t i = 0; i < TotalParams; i++) {
                std::string & ParamName = Procedure.ParametersNames[i];
                BoxedValue Data = Block.GetInputData(i);
                /* off you go little one */
                ParamBindings[ParamName] = std::move(Data);
            }

            CurrentScript->StackPush({
                .ReturnId = Block.GetNextKey(),
                .RepeatId = {},
                .RepeatCondition = double(-1),
                .InsideProcedure = CurrentScript->GetControlFlag(SCRIPT_INSIDE_PROCEDURE) 
            });

            CurrentScript->GetParameterBindings().emplace_back(std::move(ParamBindings));

            CurrentScript->SetControlFlag((SCRIPT_SHOULD_STAY | SCRIPT_INSIDE_PROCEDURE | SCRIPT_INVALIDATE_BLOCK));
            CurrentScript->CurrentBlockId = ProcBlock->GetNextKey();
            return ScratchStatus::SCRATCH_NEXT;
        }
    }
    DebugError("Invalid procedure call!\n");
    return ScratchStatus::SCRATCH_END;
}

/**
 * Compiler implementation for 'procedures_call'
 */
static ScratchStatus __hot c_ProceduresCall(TachyonAssembler & __unused Asm, ScratchBlock & Block) {
    const ScratchMutation & Mutation = Block.GetMutation();
    ScratchSprite & Owner = Block.GetOwnerSprite();

    for (auto & Procedure : Owner.Procedures) {
        /* find matching procedure */
        if (Procedure.ProcCode == Mutation.ProcCode) {
            ScratchScript * CurrentScript = Tachyon::GetCurrentScript();
            TachyonAssert(CurrentScript != nullptr);

            /* unbox and bind parameters */

            size_t TotalParams = Procedure.ParametersNames.size();
            ProcedureBindings ParamBindings;
            ParamBindings.reserve(TotalParams);

            for(size_t i = 0; i < TotalParams; i++) {
                std::string & ParamName = Procedure.ParametersNames[i];
                BoxedValue Data = Block.GetInputData(i);
                // Asm.EmitValueLoad(Data);
            }

            Tachyon::CompileProcedure(Procedure, *CurrentScript, Procedure.DefinitionKey);

            Procedure.JITData.CodeEntry();

            /* TODO: deallocate memory after vm completely stops execution*/

            return ScratchStatus::SCRATCH_NEXT;
        }
    }
    DebugError("Invalid procedure call!\n");
    return ScratchStatus::SCRATCH_END;
}

static ScratchStatus __hot ProceduresDefinition(TachyonAssembler & Asm, ScratchBlock & Block) {
    const ScratchInput & Input = Block.GetInput(0);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    TachyonAssertMsg(std::holds_alternative<std::string>(Input.Input) == true, "Invalid procedure definition!\n");

    ScratchBlock * ProcedurePrototype = Owner.GetBlockFromId(std::get<std::string>(Input.Input));

    if (unlikely(ProcedurePrototype == nullptr)) {
        DebugError("Procedure definition has an invalid prototype!\n");
        return ScratchStatus::SCRATCH_END;
    }
    
    const ScratchMutation & Mutation = ProcedurePrototype->GetMutation();

    for (auto & Procedure : Owner.Procedures) {
        /* find matching procedure */
        if (Procedure.ProcCode == Mutation.ProcCode) {
            /* bind parameters */
        }
    }

    return ScratchStatus::SCRATCH_NEXT;
}

void Procedures::RegisterAll(void) {
    /* compiler */
    Tachyon::RegisterCompileHandler("procedures_call", c_ProceduresCall);
    /* interpreter */
    Tachyon::RegisterOpHandler("procedures_call", i_ProceduresCall);
}
