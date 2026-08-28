#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Looks.hpp>
#include <Lib/NanBox.hpp>

using namespace NanBox;
using namespace Scratch;

static ScratchStatus i_Looks_Say(ScratchBlock & Block) {
    BoxedValue Data = Block.GetInputData(0);
    std::cout << "looks_say: " << Data << std::endl;
    return ScratchStatus::SCRATCH_NEXT;
}

extern "C" void Scratch::RuntimeSay(const NanBox::BoxedValue Value) {
    std::cout << "looks_say: " << NanBox::UnboxAsString(Value) << std::endl;
}

void Looks::RegisterAll(void) {
    Tachyon::RegisterOpHandler("looks_say", i_Looks_Say);
}
