#include <Scratch/BlockFields.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>
#include <Tachyon/Compiler.hpp>
#include <Tachyon/Scheduler.hpp>
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

/**
 * A pointer to the Stage sprite.
 */
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
    /* interpreter opcode handler */
    auto OpItem = OpcodeHandlers.find(this->Opcode);
    if (OpItem != OpcodeHandlers.end()) {
        this->Handler = OpItem->second;
    }
    /* interpreter reporter handler */
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

/**
 * @returns The stage sprite
 */
ScratchSprite * __hot Tachyon::GetStage(void) {
    return Stage;
}

/**
 * Sets the stage sprite used in variable/list lookups
 * @param Sprite The stage sprite
 */
void Tachyon::SetStage(ScratchSprite * Sprite) {
    Stage = Sprite;
}

/**
 * @returns The current running script
 */
ScratchScript * __hot Tachyon::GetCurrentScript(void) {
    return CurrentScript;
}

/**
 * Prepares the scheduler for execution
 * @param Project The project
 */
void Tachyon::InitializeScheduler(ScratchProject & Project) {
    Tachyon::GetVM()->Project = &Project;
    /* all scripts are BORN ready */
    for(auto & Sprite : Project.Sprites) {
        for(auto & Script : Sprite->Scripts) {
            SchedulerRunQueue.emplace_back(Script);
        }
    }
    SchedulerYieldQueue.resize(SchedulerRunQueue.size());
    if ((Tachyon::GetConfigVM() & TACHYON_CFG_PBLOCK) == false) {
        DebugWarn("Pseudo-blocks are disabled. This isn't a problem for projects that don't support Tachyon, but for those that do, you may notice a drop in performance and memory efficiency.\n");
    }
    DebugInfo("String pool size: %d\n", GarbageCollector::GetNumStrings());
}

void __hot Tachyon::ScriptAddReadyQueue(ScratchScript & Script) {
    SchedulerRunQueue.emplace_back(Script);
}


static inline ScratchScript * __hot GetNextScript(void) {
    if (likely(SchedulerRunQueue.empty() == false)) {
        return &SchedulerRunQueue.front();
    }
    return nullptr;
} 

/**
 * Steps once into the script.
 * Can either execute in native machine code, or in interpreted mode.
 * @returns False if the script executed successfully, true if otherwise. 
 */
bool __hot Tachyon::Step(void) {
    /* NOTE: waking up scripts isnt implemeneted, so they'll be sleeping beauty for the time being */
    CurrentScript = GetNextScript();
    if (unlikely(CurrentScript == nullptr)) {
        return true;
    }
    ScratchStatus Status = ScratchStatus::SCRATCH_NEXT;
    Tachyon::OutputCodeInfo & CodeInfo = CurrentScript->JITState.CodeInfo;
    if (CodeInfo.CodeEntry == nullptr) {
        // DebugInfo("Script is not yet compiled. Compiling...\n");
        CurrentScript->JITState.CodeInfo = Tachyon::Compile(*CurrentScript, CurrentScript->FirstBlockId);
        return false;
    } else {
        /* flush instruction cache */
        Tachyon::PrepareCPUCache(reinterpret_cast<void *>(CodeInfo.CodeEntry), CodeInfo.CodeSize);
        Status = (ScratchStatus)CodeInfo.CodeEntry();
        DebugInfo("Script returned with status %u\n", Status);
        /* deallocate */
        CodeInfo.FreeMemory();
    }
    switch(Status) {
        case ScratchStatus::SCRATCH_END: {
            SchedulerRunQueue.erase(SchedulerRunQueue.begin());
            break;
        }
        default: {
            break;
        }
    }
    return false;
}
