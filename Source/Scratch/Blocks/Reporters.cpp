#include <Scratch/Common.hpp>
#include <Scratch/Reporters.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Lib/NanBox.hpp>
#include <string>

using namespace NanBox;
using namespace Scratch;

static inline BoxedValue __hot GetBindedParameter(std::string ParamName, ScratchScript & CurrentScript) {
    TachyonAssert(CurrentScript.GetParameterBindings().empty() == false);

    ProcedureBindings Map = CurrentScript.GetParameterBindings().back();

    auto Item = Map.find(ParamName);

    if (unlikely(Item == Map.end())) {
        return double(0);
    }

    return Item->second;
}

static BoxedValue __hot Reporter_Boolean(ScratchBlock & Block) {
    const ScratchField & Field = Block.GetField(0);
    std::string ParamName = std::get<std::string>(Field.Field);
    if (ParamName == "Is Tachyon?") {
        return true;
    }
    return GetBindedParameter(ParamName, *Tachyon::GetCurrentScript());
}

static __hot BoxedValue Reporter_StringNum(ScratchBlock & Block) {
    const ScratchField & Field = Block.GetField(0);
    std::string ParamName = std::get<std::string>(Field.Field);
    return GetBindedParameter(ParamName, *Tachyon::GetCurrentScript());
}

void Reporters::RegisterAll(void) {
    Tachyon::RegisterEvaluationHandler("argument_reporter_boolean", Reporter_Boolean);
    Tachyon::RegisterEvaluationHandler("argument_reporter_string_number", Reporter_StringNum);
}
