#include <Scratch/Common.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>
#include <Lib/NanBox.hpp>
#include <cstdint>

using namespace NanBox;
using namespace Tachyon;
using namespace Scratch;

/**
 * Map containing all pseudo-block handlers.
 */
static std::unordered_map<std::string_view, OpcodeHandler> PseudoHandlers;

bool Pseudo::IsPseudo(std::string ProcCode) {
    return (PseudoHandlers.find(ProcCode) != PseudoHandlers.end());
}

static ScratchStatus __hot Tachyon_LoadU8Buffer(ScratchBlock & Block) {
    BoxedValue Name = Block.GetInputData(0);
    ScratchSprite & Owner = Block.GetOwnerSprite();

    std::string ListName = UnboxAsString(Name);
    ScratchList * List = Owner.GetList(ListName);
    if (List != nullptr) {
        List->SwitchToBuffer();
        return ScratchStatus::SCRATCH_NEXT;
    }
    DebugWarn("Failed to load buffer: Invalid name: \"%s\"\n", ListName.data());
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Tachyon_Xor(ScratchBlock & Block) {
    BoxedValue InputA = Block.GetInputData(0);
    BoxedValue InputB = Block.GetInputData(1);

    ScratchSprite & Owner = Block.GetOwnerSprite();

    ScratchVariable * Result = Owner.GetVariable("XOR Return");

    if (unlikely(Result == nullptr)) {
        DebugError("XOR Result variable does not exist. Please create it.\n");
        return ScratchStatus::SCRATCH_END;
    }

    const uint32_t A = static_cast<uint32_t>(UnboxAsDouble(InputA));
    const uint32_t B = static_cast<uint32_t>(UnboxAsDouble(InputB));

    Result->SetData(std::move(Box(static_cast<double>(A ^ B))));

    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Tachyon_Log(ScratchBlock & Block) {
    BoxedValue String = Block.GetInputData(0);
    std::cout << String << std::endl;
    return ScratchStatus::SCRATCH_NEXT;
}

ScratchStatus __hot Pseudo::Execute(std::string ProcCode, Scratch::ScratchBlock & Block) {
    TachyonAssert(Pseudo::IsPseudo(ProcCode) == true);
    auto Item = PseudoHandlers.find(ProcCode);
    return Item->second(Block);
}

static void RegisterPseudoHandler(std::string ProcCode, OpcodeHandler Handler) {
    TachyonAssert(PseudoHandlers.find(ProcCode) == PseudoHandlers.end());
    PseudoHandlers.emplace(ProcCode, Handler);
}

void Pseudo::RegisterAll(void) {
    RegisterPseudoHandler("Tachyon: Load large UINT8 buffer %s", Tachyon_LoadU8Buffer);
    RegisterPseudoHandler("Tachyon: Log %s", Tachyon_Log);
    RegisterPseudoHandler("Tachyon: XOR32 %s %s", Tachyon_Xor);
}
