#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Compiler.hpp>
#include <Tachyon/Events.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Debug.hpp>

using namespace Scratch;

/**
 * Interpreter implementations ----
 */

static ScratchStatus EventBroadcastAndWait(ScratchBlock & Block) {
    ScratchSprite & Owner = Block.GetOwnerSprite();
    /* TODO: do the same thing as EventBroadcast but instead of creating a seperate script, push it in the call stack */
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus EventBroadcast(ScratchBlock & Block) {
    ScratchProject & Project = *Tachyon::GetLoadedProject();
    const ScratchInput & Input = Block.GetInput(0);
    if (Input.IsType(InputType::BroadcastInput) == false) {
        return ScratchStatus::SCRATCH_END;
    }
    const std::string BroadcastInputKey = std::get<std::string>(Input.Input);
    /* search in the sprite */
    for(auto & Sprite : Project.Sprites) {
        for(const auto & Broadcast : Sprite->BroadcastReceivers) {
            ScratchBlock BroadcastBlock = *Broadcast.second;
            const ScratchField & Field = BroadcastBlock.GetField(0);
            if (Field.IsType(FieldType::BroadcastOption) == false) {
                continue;
            }
            const std::string BroadcastFieldKey = std::get<std::string>(Field.Field);
            if (BroadcastInputKey == BroadcastFieldKey) {
                std::string NextBroadcastKey = BroadcastBlock.GetKey();
                if (NextBroadcastKey.empty() == true) {
                    /* nothing to do for the broadcast */
                    continue;
                }
                ScratchBlock * NextBroadcastBlock = Sprite->GetBlockFromId(NextBroadcastKey);
                if (NextBroadcastBlock == nullptr) {
                    /* bad broadcast */
                    continue;
                }
                Sprite->CreateScript(*NextBroadcastBlock);
            }
        }
    }
    return ScratchStatus::SCRATCH_NEXT;
}
 
void Events::RegisterAll(void) {
    /* interpreter */
    Tachyon::RegisterOpHandler("event_broadcast", EventBroadcast);
    Tachyon::RegisterOpHandler("event_broadcastandwait", EventBroadcastAndWait);
}
