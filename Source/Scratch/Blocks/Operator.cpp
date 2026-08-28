#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Operator.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Lib/NanBox.hpp>
#include <Common.hpp>
#include <random>
#include <cmath>
#include <limits>

using namespace NanBox;
using namespace Scratch;

static BoxedValue __hot Operator_Add(ScratchBlock & Block) {
    BoxedValue Operand1_Data = Block.GetInputData(0);
    BoxedValue Operand2_Data = Block.GetInputData(1);

    return Box(UnboxAsDouble(Operand1_Data) + UnboxAsDouble(Operand2_Data));
}

static BoxedValue __hot Operator_Subtract(ScratchBlock & Block) {
    BoxedValue Operand1_Data = Block.GetInputData(0);
    BoxedValue Operand2_Data = Block.GetInputData(1);

    return Box(UnboxAsDouble(Operand1_Data) - UnboxAsDouble(Operand2_Data));
}

static BoxedValue __hot Operator_Modulo(ScratchBlock & Block) {
    BoxedValue Operand1_Data = Block.GetInputData(0);
    BoxedValue Operand2_Data = Block.GetInputData(1);

    return Box(std::fmod(
        UnboxAsDouble(Operand1_Data),
        UnboxAsDouble(Operand2_Data)
    ));
}

static BoxedValue __hot Operator_MathOp(ScratchBlock & Block) {
    const ScratchField & Field = Block.GetField(0);

    TachyonAssert(Field.IsType(FieldType::StringField));

    const std::string Operation = std::get<std::string>(Field.Value);

    if (Operation == "floor") {
        BoxedValue Data = Block.GetInputData(0);
        return Box(std::floor(UnboxAsDouble(Data)));
    }

    DebugError("Invalid math operation: %s\n", Operation.c_str());

    return Box(0.0);
}

static BoxedValue __hot Operator_Divide(ScratchBlock & Block) {
    BoxedValue Left = Block.GetInputData(0);
    BoxedValue Right = Block.GetInputData(1);

    const double Operand1 = UnboxAsDouble(Left);
    const double Operand2 = UnboxAsDouble(Right);

    if (Operand2 == 0) {
        if (Operand1 == 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return Box((Operand1 < 0) ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity());
    }
    return Box(Operand1 / Operand2);
}

static BoxedValue __hot Operator_Multiply(ScratchBlock & Block) {
    BoxedValue Left = Block.GetInputData(0);
    BoxedValue Right = Block.GetInputData(1);

    const double Operand1 = UnboxAsDouble(Left);
    const double Operand2 = UnboxAsDouble(Right);

    return Box(Operand1 * Operand2);
}

static BoxedValue __hot Operator_Random(ScratchBlock & Block) {
    BoxedValue Left = Block.GetInputData(0);
    BoxedValue Right = Block.GetInputData(1);

    const double LeftNum = UnboxAsDouble(Left);
    const double RightNum = UnboxAsDouble(Right);

    if (LeftNum == RightNum) { return Left; }

    const double Low = std::min(LeftNum, RightNum);
    const double Max = std::max(LeftNum, RightNum);

    static std::mt19937 Generator(std::random_device{ /* empty */ }());
    std::uniform_real_distribution<double> Dist(Low, Max);

    return Box(Dist(Generator));
}

static BoxedValue __hot Operator_Join(ScratchBlock & Block) {
    BoxedValue Data1 = Block.GetInputData(0);
    BoxedValue Data2 = Block.GetInputData(1);

    const std::string String1 = UnboxAsString(Data1);
    const std::string String2 = UnboxAsString(Data2);

    return Box(String1 + String2);
}

static BoxedValue __hot Operator_Equals(ScratchBlock & Block) {
    BoxedValue Data1 = Block.GetInputData(0);
    BoxedValue Data2 = Block.GetInputData(1);

    const std::string String1 = UnboxAsString(Data1);
    const std::string String2 = UnboxAsString(Data2);

    return Box(String1 == String2);
}

static BoxedValue __hot Operator_Not(ScratchBlock & Block) {
    const ScratchInput & Condition = Block.GetInput(0);
    TachyonAssert(Condition.IsType(InputType::ValueInput));

    std::string ConditionBlockId = std::get<std::string>(Condition.Input);

    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchBlock * Reporter = Owner.GetBlockFromId(ConditionBlockId);

    TachyonAssert(Reporter != nullptr);

    const BoxedValue Evaluation = Reporter->Evaluate();
    return Box(UnboxAsBoolean(Evaluation) == false);
}

static BoxedValue __hot Operator_Or(ScratchBlock & Block) {
    const ScratchInput ConditionA = Block.GetInput(0);
    const ScratchInput ConditionB = Block.GetInput(1);
    
    TachyonAssert(ConditionA.IsType(InputType::ValueInput));
    TachyonAssert(ConditionB.IsType(InputType::ValueInput));

    const std::string ConditionA_BlockId = std::get<std::string>(ConditionA.Input);
    const std::string ConditionB_BlockId = std::get<std::string>(ConditionB.Input);

    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchBlock * ReporterA = Owner.GetBlockFromId(ConditionA_BlockId);
    ScratchBlock * ReporterB = Owner.GetBlockFromId(ConditionB_BlockId);

    TachyonAssert(ReporterA != nullptr);
    TachyonAssert(ReporterB != nullptr);

    const BoxedValue EvaluationA = ReporterA->Evaluate();
    const BoxedValue EvaluationB = ReporterB->Evaluate();

    return Box(UnboxAsBoolean(EvaluationA) || UnboxAsBoolean(EvaluationB));
}

static BoxedValue __hot Operator_Length(ScratchBlock & Block) {
    BoxedValue Data = Block.GetInputData(0);
    return Box(static_cast<double>(UnboxAsString(Data).length()));
}

static BoxedValue __hot Operator_LetterOf(ScratchBlock & Block) {
    BoxedValue IndexInput = Block.GetInputData(0);
    BoxedValue Input = Block.GetInputData(1);

    if (unlikely(HoldsType<double>(IndexInput))) {
        return Box("");
    }

    const double Index = UnboxAsDouble(IndexInput);
    const std::string RealString = UnboxAsString(Input);

    if (unlikely(Index < 0 || RealString.length() < Index)) {
        return Box("");
    }

    std::string Character(1, RealString[Index - 1]);
    return Box(Character);
}

void Operator::RegisterAll(void) {
    /* arithmetic related */
    Tachyon::RegisterEvaluationHandler("operator_add", Operator_Add);
    Tachyon::RegisterEvaluationHandler("operator_subtract", Operator_Subtract);
    Tachyon::RegisterEvaluationHandler("operator_equals", Operator_Equals);
    Tachyon::RegisterEvaluationHandler("operator_or", Operator_Or);
    Tachyon::RegisterEvaluationHandler("operator_not", Operator_Not);
    Tachyon::RegisterEvaluationHandler("operator_mod", Operator_Modulo);
    Tachyon::RegisterEvaluationHandler("operator_mathop", Operator_MathOp);
    Tachyon::RegisterEvaluationHandler("operator_divide", Operator_Divide);
    Tachyon::RegisterEvaluationHandler("operator_multiply", Operator_Multiply);
    Tachyon::RegisterEvaluationHandler("operator_random", Operator_Random);
    /* string manipulation */
    Tachyon::RegisterEvaluationHandler("operator_length", Operator_Length);
    Tachyon::RegisterEvaluationHandler("operator_letter_of", Operator_LetterOf);
    Tachyon::RegisterEvaluationHandler("operator_join", Operator_Join);
}
