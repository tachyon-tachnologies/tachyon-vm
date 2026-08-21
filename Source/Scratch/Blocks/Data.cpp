#include <Tachyon/Debug.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Data.hpp>
#include <Tachyon/Tachyon.hpp>

using namespace Scratch;
using namespace NanBox;

/*
    Interpreter data handling
*/

static ScratchStatus __hot Data_DeleteAllOfList(ScratchBlock & Block) {
    const ScratchField & Field = Block.GetField(0);
    TachyonAssert(Field.IsType(FieldType::ListField));

    ScratchList * List = std::get<ScratchList *>(Field.Field);

    List->ClearElements();
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Data_AddToList(ScratchBlock & Block) {
    BoxedValue Data = Block.GetInputData(0);
    const ScratchField & Field = Block.GetField(0);
    TachyonAssert(Field.IsType(FieldType::ListField));

    ScratchList * List = std::get<ScratchList *>(Field.Field);

    List->Append(Data);
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Data_ReplaceItem(ScratchBlock & Block) {
    BoxedValue Index = Block.GetInputData(0);
    BoxedValue Data = Block.GetInputData(1);
    const ScratchField & Field = Block.GetField(0);

    TachyonAssert(Field.IsType(FieldType::ListField));

    ScratchList * List = std::get<ScratchList *>(Field.Field);

    if (unlikely(HoldsType<std::string>(Index))) {
        std::string & IndexString = UnboxString(Index);
        if (IndexString == "last") {
            List->Set(Data, List->TotalItems - 1);
        } else {
            DebugWarn("Invalid item replace index string.\n");
        }
        return ScratchStatus::SCRATCH_NEXT;
    }

    double IndexNum = UnboxAsDouble(Index);
    if (IndexNum < 0) {
        return ScratchStatus::SCRATCH_NEXT;
    }
    List->Set(Data, IndexNum - 1);
    return ScratchStatus::SCRATCH_NEXT;
}

static BoxedValue __hot Data_ItemOfList(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);
    BoxedValue Index = Block.GetInputData(0);

    TachyonAssert(Field.IsType(FieldType::ListField));

    ScratchList * List = std::get<ScratchList *>(Field.Field);

    double IndexNum = UnboxAsDouble(Index); 
    if (IndexNum < 0) {
        return Box("");
    }

    return List->Get(IndexNum - 1);
}

static BoxedValue __hot Data_LengthOfList(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.IsType(FieldType::ListField));

    ScratchList * List = std::get<ScratchList *>(Field.Field);

    return Box(static_cast<double>(List->TotalItems));
}

static ScratchStatus __hot Data_SetVariable(ScratchBlock & Block) {
    const ScratchField & Field = Block.GetField(0);

    TachyonAssert(Field.IsType(FieldType::VariableField));

    ScratchVariable * Variable = std::get<ScratchVariable *>(Field.Field);
    BoxedValue Data = Block.GetInputData(0);
    Variable->SetData(Data);
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Data_ChangeVariableBy(ScratchBlock & Block) {
    const ScratchField & Field = Block.GetField(0);

    TachyonAssert(Field.IsType(FieldType::VariableField));

    ScratchVariable * Variable = std::get<ScratchVariable *>(Field.Field);

    BoxedValue Num = Block.GetInputData(0);
    BoxedValue Data = Variable->GetData();

    double X = UnboxAsDouble(Data);
    double Y = UnboxAsDouble(Num);

    Variable->SetData(Box(X + Y));

    return ScratchStatus::SCRATCH_NEXT;
}

void Data::RegisterAll(void) {
    Tachyon::RegisterOpHandler("data_deletealloflist", Data_DeleteAllOfList);
    Tachyon::RegisterOpHandler("data_setvariableto", Data_SetVariable);
    Tachyon::RegisterOpHandler("data_addtolist", Data_AddToList);
    Tachyon::RegisterOpHandler("data_changevariableby", Data_ChangeVariableBy);
    Tachyon::RegisterOpHandler("data_replaceitemoflist", Data_ReplaceItem);
    Tachyon::RegisterEvaluationHandler("data_itemoflist", Data_ItemOfList);
    Tachyon::RegisterEvaluationHandler("data_lengthoflist", Data_LengthOfList);
}
