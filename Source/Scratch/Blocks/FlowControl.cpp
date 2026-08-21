#include <Scratch/Data.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/ControlFlow.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Scheduler.hpp>
#include <Tachyon/Debug.hpp>
#include <Lib/NanBox.hpp>
#include <Common.hpp>

using namespace NanBox;
using namespace Scratch;

static ScratchStatus __hot ControlFlow_If(ScratchBlock & Block) {
    const ScratchInput & Condition = Block.GetInput(0);
    const ScratchInput & Substack = Block.GetInput(1);
    
    if (unlikely(Condition.IsType(InputType::InvalidInput))) {
        /* no condition is always false. advance to the next block */
        return ScratchStatus::SCRATCH_NEXT;
    }

    TachyonAssert(Substack.IsType(InputType::SubstackInput) || Condition.IsType(InputType::ConditionInput));

    ScratchSprite & Owner = Block.GetOwnerSprite();
    std::string ConditionBlockId = std::get<std::string>(Condition.Input);
    std::string SubstackFirstBlock = std::get<std::string>(Substack.Input);

    if (unlikely(SubstackFirstBlock.empty() == true)) {
        /* will always equate to false anyways */
        return ScratchStatus::SCRATCH_NEXT;
    }

    BoxedValue Evaluation = Owner.GetBlockFromId(ConditionBlockId)->Evaluate();
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();

    if (UnboxAsBoolean(Evaluation) == true) {
        CurrentScript->StackPush({
            .ReturnId = Block.GetNextKey(),
            .RepeatId = {},
            .RepeatCondition = double(-1),
            .InsideProcedure = bool(CurrentScript->GetControlFlag(SCRIPT_INSIDE_PROCEDURE)) 
        });
        CurrentScript->CurrentBlockId = SubstackFirstBlock;
        CurrentScript->SetControlFlag(SCRIPT_SHOULD_STAY);
    }
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot ControlFlow_IfElse(ScratchBlock & Block) {
    const ScratchInput & Condition = Block.GetInput(0);
    const ScratchInput & SubstackIf = Block.GetInput(1);
    const ScratchInput & SubstackElse = Block.GetInput(2);
    
    if (unlikely(Condition.IsType(InputType::InvalidInput))) {
        /* no condition is always false. advance to the next block */
        return ScratchStatus::SCRATCH_NEXT;
    }

    TachyonAssert(
        SubstackIf.IsType(InputType::SubstackInput) ||
        SubstackElse.IsType(InputType::SubstackInput) ||
        Condition.IsType(InputType::ConditionInput)
    );

    ScratchSprite & Owner = Block.GetOwnerSprite();
    std::string ConditionBlockId = std::get<std::string>(Condition.Input);
    std::string SubstackIfFirstBlock = std::get<std::string>(SubstackIf.Input);
    std::string SubstackElseFirstBlock = std::get<std::string>(SubstackElse.Input);

    BoxedValue Evaluation = Owner.GetBlockFromId(ConditionBlockId)->Evaluate();
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();

    /* we're going into another stack whether we like it or not */
    CurrentScript->StackPush({
        .ReturnId = Block.GetNextKey(),
        .RepeatId = {},
        .RepeatCondition = double(-1),
        .InsideProcedure = bool(CurrentScript->GetControlFlag(SCRIPT_INSIDE_PROCEDURE)) 
    });

    CurrentScript->CurrentBlockId = UnboxAsBoolean(Evaluation) ? SubstackIfFirstBlock : SubstackElseFirstBlock;
    CurrentScript->SetControlFlag(SCRIPT_SHOULD_STAY);
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot ControlFlow_Stop(ScratchBlock & Block) {
    /* just retire the thread for now */
    ScratchMutation & Mutation = Block.GetMutation();
    const ScratchField & StopOption = Block.GetField(0);
    
    TachyonAssert(StopOption.IsType(FieldType::StringField));

    std::string Option(std::get<std::string>(StopOption.Field));
    if (Option == "this script") {
        return ScratchStatus::SCRATCH_END;
    } else if (Option == "other scripts in sprite") {
        DebugInfo("TODO: Retire all but the current script\n");
        return ScratchStatus::SCRATCH_NEXT;
    } else {
        DebugInfo("TODO: Retire every script\n");
        return ScratchStatus::SCRATCH_END;
    }
    return ScratchStatus::SCRATCH_END;
}

static ScratchStatus __hot ControlFlow_Repeat(ScratchBlock & Block) {
    const ScratchInput & Substack = Block.GetInput(0);
    const BoxedValue TimesInput = Block.GetInputData(1);

    TachyonAssert(Substack.IsType(InputType::SubstackInput));
    
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();
    std::string SubstackFirstBlock = std::get<std::string>(Substack.Input);

    if (unlikely(SubstackFirstBlock.empty() == true)) {
        return ScratchStatus::SCRATCH_NEXT;
    }

    const double Times = UnboxAsDouble(TimesInput);

    CurrentScript->StackPush({
        .ReturnId = Block.GetNextKey(),
        .RepeatId = SubstackFirstBlock,
        .RepeatCondition = Times - 1,
        .InsideProcedure = bool(CurrentScript->GetControlFlag(SCRIPT_INSIDE_PROCEDURE)) 
    });
    CurrentScript->CurrentBlockId = SubstackFirstBlock;
    CurrentScript->SetControlFlag(SCRIPT_INVALIDATE_BLOCK | SCRIPT_SHOULD_STAY);
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot ControlFlow_While(ScratchBlock & Block) {
    const ScratchInput & Condition = Block.GetInput(0);
    const ScratchInput & Substack = Block.GetInput(1);

    if (unlikely(Condition.IsType(InputType::InvalidInput))) {
        /* nothing to execute if the condition is false */
        return ScratchStatus::SCRATCH_NEXT;
    }

    TachyonAssert(Substack.IsType(InputType::SubstackInput) && Condition.IsType(InputType::ConditionInput));

    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();

    std::string SubstackFirstBlock = std::get<std::string>(Substack.Input);
    std::string ConditionBlockId = std::get<std::string>(Condition.Input);

    ScratchBlock * ConditionBlock = Owner.GetBlockFromId(ConditionBlockId);

    TachyonAssert(ConditionBlock != nullptr);

    if (unlikely(SubstackFirstBlock.empty() == true)) {
        return ScratchStatus::SCRATCH_NEXT;
    }
    CurrentScript->StackPush({
        .ReturnId = Block.GetNextKey(),
        .RepeatId = SubstackFirstBlock,
        .RepeatCondition = ConditionBlock,
        .InsideProcedure = bool(CurrentScript->GetControlFlag(SCRIPT_INSIDE_PROCEDURE)) 
    });
    CurrentScript->CurrentBlockId = SubstackFirstBlock;
    CurrentScript->SetControlFlag(SCRIPT_INVALIDATE_BLOCK | SCRIPT_SHOULD_STAY);
    return ScratchStatus::SCRATCH_NEXT;
}

void ControlFlow::RegisterAll(void) {
    Tachyon::RegisterOpHandler("control_if", ControlFlow_If);
    Tachyon::RegisterOpHandler("control_if_else", ControlFlow_IfElse);
    Tachyon::RegisterOpHandler("control_stop", ControlFlow_Stop);
    Tachyon::RegisterOpHandler("control_repeat", ControlFlow_Repeat);
    Tachyon::RegisterOpHandler("control_while", ControlFlow_While);
}
