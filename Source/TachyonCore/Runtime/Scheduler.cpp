#include <Scratch/BlockFields.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>
#include <string_view>
#include <deque>

using namespace Scratch;

/**
 * A queue containing runnable threads/scripts.
 */
static std::deque<ScratchScript> SchedulerRunQueue;

/**
 * A queue containing blocked threads/scripts.
 */
static std::deque<ScratchScript> SchedulerYieldQueue;

static ScratchScript * CurrentScript = nullptr;
static ScratchSprite * Stage = nullptr;

/**
 * Map containing all opcode handlers.
 */
static std::unordered_map<std::string_view, OpcodeHandler> OpcodeHandlers;

/**
 * Map containing all reporter handlers.
 */
static std::unordered_map<std::string_view, EvaluationHandler> ReporterHandlers;

void ScratchBlock::LinkHandlers(void) {
    auto OpItem = OpcodeHandlers.find(this->Opcode);
    if (OpItem != OpcodeHandlers.end()) {
        this->Handler = OpItem->second;
    }
    auto EvalItem = ReporterHandlers.find(this->Opcode);
    if (EvalItem != ReporterHandlers.end()) {
        this->ReporterHandler = EvalItem->second;
    }
}

void Tachyon::RegisterOpHandler(std::string_view Opcode, OpcodeHandler Handler) {
    TachyonAssert(OpcodeHandlers.find(Opcode) == OpcodeHandlers.end());
    OpcodeHandlers.emplace(Opcode, Handler);
}

void Tachyon::RegisterEvaluationHandler(std::string_view Opcode, EvaluationHandler Handler) {
    TachyonAssert(ReporterHandlers.find(Opcode) == ReporterHandlers.end());
    ReporterHandlers.emplace(Opcode, Handler);
}

/* tachyon stuff */

ScratchSprite * __hot Tachyon::GetStage(void) {
    return Stage;
}

/* scheduler stuff */

ScratchScript * __hot Tachyon::GetCurrentScript(void) {
    return CurrentScript;
}

void Tachyon::InitializeScheduler(ScratchProject & Project) {
    Tachyon::GetVM()->Project = &Project;
    /* all scripts are BORN ready */
    for(auto & Sprite : Project.Sprites) {
        for(auto & Script : Sprite->Scripts) {
            SchedulerRunQueue.emplace_back(Script);
        }
        if (Sprite->IsStage() == true) {
            Stage = Sprite.get();
        }
    }
    SchedulerYieldQueue.resize(SchedulerRunQueue.size());
    if ((Tachyon::GetConfigVM() & TACHYON_CFG_PBLOCK) == false) {
        DebugWarn("Pseudo-blocks are disabled. This likely isn't a problem for projects that don't support Tachyon, but for those that do, you may notice a drop in performance and memory efficiency.\n");
    }
}

void __hot Tachyon::ScriptAddReadyQueue(ScratchScript Script) {
    SchedulerRunQueue.emplace_back(Script);
}


static inline ScratchScript * __hot GetNextScript(void) {
    if (likely(SchedulerRunQueue.empty() == false)) {
        return &SchedulerRunQueue.front();
    }
    return nullptr;
} 

bool __hot Tachyon::Step(void) {
    /* NOTE: waking up scripts isnt implemeneted, so they'll be sleeping beauty for the time being */
    CurrentScript = GetNextScript();
    if (unlikely(CurrentScript == nullptr)) {
        return true;
    }
    if (CurrentScript->JIT_State.EntryPoint == nullptr) {
        DebugInfo("Script is not yet compiled. Compiling...\n");
        // TODO: compile script into native machine code
    } else {
        DebugInfo("Executing compiled script...\n");
        // nothing for now :(
    }
    return false;
}
