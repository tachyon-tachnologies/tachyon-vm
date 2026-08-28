#include <Tachyon/Debug.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Scheduler.hpp>
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

        ScratchBlock * const Prototype = this->GetBlockFromId(Procedure.PrototypeKey);
        const ScratchMutation & Mutation = Prototype->GetMutation();

        Procedure.UseWarp = Mutation.UseWarp;
        Procedure.ProcCode = Mutation.ProcCode;
        Procedure.ParametersNames = Mutation.ParametersNames;
        Procedure.ParametersKeys = Mutation.ParametersKeys;

        this->Procedures.emplace(Procedure.ProcCode, Procedure);
    }
}

/**
 * Interpreter implementation for 'procedures_call'
 */
static ScratchStatus __hot i_ProceduresCall(ScratchBlock & Block) {
    const ScratchMutation & Mutation = Block.GetMutation();
    ScratchSprite & Owner = Block.GetOwnerSprite();
    auto SearchResult = Owner.Procedures.find(Mutation.ProcCode);
    if (unlikely(SearchResult == Owner.Procedures.end())) {
        DebugError("Error: Invalid procedure: %s\n", Mutation.ProcCode);
        return ScratchStatus::SCRATCH_END;
    }
    ScratchProcedure & Procedure = SearchResult->second;
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();
    TachyonAssert(CurrentScript != nullptr);
    if (Tachyon::GetConfigVM() & TACHYON_CFG_PBLOCK) {
        if (Tachyon::Pseudo::IsPseudo(Procedure.ProcCode) == true) {
            /* act as if nothing ever happened */
            CurrentScript->CurrentBlockId = Block.GetNextKey();
            return Tachyon::Pseudo::Execute(Procedure.ProcCode, Block);
        }
    }
    ScratchBlock * const ProcBlock = Owner.GetBlockFromId(Procedure.DefinitionKey);
    TachyonAssert(ProcBlock != nullptr);

    /* bind parameters */
    size_t TotalParams = Procedure.ParametersNames.size();
    ProcedureBindings ParamBindings;
    ParamBindings.reserve(TotalParams);

    for(size_t i = 0; i < TotalParams; i++) {
        const std::string & ParamName = Procedure.ParametersNames[i];
        const BoxedValue Data = Block.GetInputData(i);
        /* off you go little one */
        ParamBindings[ParamName] = Data;
    }

    CurrentScript->StackPush({
        .ReturnId = Block.GetNextKey(),
        .RepeatId = {},
        .RepeatCondition = double(-1),
        .InsideProcedure = CurrentScript->GetControlFlag(SCRIPT_INSIDE_PROCEDURE) 
    });

    CurrentScript->GetParameterBindings().emplace_back(std::move(ParamBindings));

    CurrentScript->SetControlFlag(SCRIPT_SHOULD_STAY | SCRIPT_INSIDE_PROCEDURE | SCRIPT_INVALIDATE_BLOCK);
    CurrentScript->CurrentBlockId = ProcBlock->GetNextKey();
    return ScratchStatus::SCRATCH_NEXT;
}

void Procedures::RegisterAll(void) {
    /* interpreter */
    Tachyon::RegisterOpHandler("procedures_call", i_ProceduresCall);
}
