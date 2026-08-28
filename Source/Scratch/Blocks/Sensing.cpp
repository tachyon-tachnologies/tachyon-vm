#include "Common.hpp"
#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Sensing.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Time.hpp>
#include <Lib/NanBox.hpp>

using namespace NanBox;
using namespace Scratch;

static __hot BoxedValue Sensing_Current(ScratchBlock & Block) {
    const ScratchField & Field = Block.GetField(0);

    TachyonAssert(Field.IsType(FieldType::StringField));

    std::string What = std::get<std::string>(Field.Value);

    if (What == "YEAR") { return Box(Tachyon::Time::GetYear()); }
    if (What == "MONTH") { return Box(Tachyon::Time::GetMonth()); }
    if (What == "DATE") { return Box(Tachyon::Time::GetDayOfMonth()); }
    if (What == "DAYOFWEEK") { return Box(Tachyon::Time::GetDayOfWeek()); }
    if (What == "HOUR") { return Box(Tachyon::Time::GetHour()); }
    if (What == "MINUTE") { return Box(Tachyon::Time::GetMinute()); }
    if (What == "SECOND") { return Box(Tachyon::Time::GetSeconds()); }

    TachyonUnimplemented("What kind of option is this?? %s\n", What.c_str());

    __unreachable;
}

static __hot BoxedValue Sensing_DaysSinceY2K( [[maybe_unused]] ScratchBlock & Block) {
    return Box(Tachyon::Time::GetDaysSince2000());
}

void Sensing::RegisterAll(void) {
    //Tachyon::RegisterEvaluationHandler("sensing_timer", Sensing_Timer);
    Tachyon::RegisterEvaluationHandler("sensing_dayssince2000", Sensing_DaysSinceY2K);
    Tachyon::RegisterEvaluationHandler("sensing_current", Sensing_Current);
}
