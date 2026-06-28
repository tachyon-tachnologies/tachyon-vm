#pragma once

#include <Scratch/Procedures.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Common.hpp>
#include <string_view>
#include <cstdint>

#include <SDL3/SDL_render.h>

/**
 * Configuration option: Script watchdog
 * Enables a watchdog for all scripts.
 */
#define TACHYON_CFG_WATCHDOG    (1 << 0)

/**
 * Configuration option: Pseudo-block support
 * When enabled, procedures with certain names and structure can be called to make the VM do heavy tasks instead of the project.
 */
#define TACHYON_CFG_PBLOCK      (1 << 1)

/**
 * Configuration option: Shit talk
 * Talks shit about WarpDrive (build a x86 JIT compiler before talking shit, loser), TurboWarp (we don't mean it for yall), and any other Scratch mod that is slower than us.
 */
#define TACHYON_CFG_SHITTALK    (1 << 2)

/**
 * Configuration option: Blitz
 * Makes the VM execute code extremely fast. (no FPS cap)
 */
#define TACHYON_CFG_BLITZ       (1 << 3)

/**
 * Configuration option: Debug mode
 * Enables debugging on the Tachyon VM
 */
#define TACHYON_CFG_DEBUG       (1 << 4)

/**
 * Configuration option: Compiler
 * Enables the Tachyon JIT compiler.
 */
#define TACHYON_CFG_COMPILER    (1 << 5)

namespace Tachyon {

    using TachyonConfig = uint16_t;

    struct Tachyon_VirtualMachine {
        Scratch::ScratchProject * Project = nullptr;
        SDL_Window * TachyonWindow = nullptr;
        SDL_Renderer * TachyonRenderer = nullptr;
        TachyonConfig Configuration;
        bool ShouldExit;
        bool RendererUpdate;
    };

    /**
     * Initializes Tachyon and registers all opcode handlers.
     */
    void Init(void);

    /**
     * Initializes SDL3.
     */
    bool InitWindow(void);

    /**
     * Initializes the scheduler.
     * @param Project The scratch project to execute.
     */
    void InitializeScheduler(Scratch::ScratchProject & Project);

    /**
     * Gets the VM information.
     * @return VM information.
     */
    Tachyon_VirtualMachine * GetVM(void);

    /**
     * Gets the VM configuration.
     * @return The VM configuration.
     */
    TachyonConfig GetConfigVM(void);

    /**
     * Adds a script to the ready queue of the scheduler.
     * @param Script The script
     */
    void ScriptAddReadyQueue(Scratch::ScratchScript & Script);

    /**
     * Gets the loaded project.
     * @return The loaded project.
     */
    Scratch::ScratchProject * GetLoadedProject(void);

    /**
     * Gets the currently running script.
     * @return The script that is currently running.
     */
    Scratch::ScratchScript * GetCurrentScript(void);

    /**
     * @returns The stage sprite
     */
    Scratch::ScratchSprite * GetStage(void);
    
    /**
     * Sets the stage sprite used in variable/list lookups
     * @param Sprite The stage sprite
     */
    void SetStage(Scratch::ScratchSprite * Sprite);

    /**
     * Checks if the VM has the debugger enabled.
     * @return True if the debugger is enabled, and false if otherwise.
     */
    bool DebuggerEnabled(void);

    /**
     * Starts the main VM loop.
     */
    void __hot MainLoop(void);

    /**
     * Performs executions.
     * @param The scratch project to execute.
     * @return Returns true if the VM should exit, false if otherwise. 
     */
    bool __hot Step(void);

    /**
     * Renders sprites and anything else.
     */
    void __hot Render(void);

    /**
     * Registers a block opcode handler (for the regular interpreter only).
     * @param Opcode The block's opcode
     * @param Handler The handler function.
     */
    void RegisterOpHandler(std::string_view Opcode, Scratch::OpcodeHandler Handler);

    /**
     * Registers a block evaluation handler (for the regular interpreter only).
     * @param Opcode The block's opcode
     * @param Handler The handler function
     */
    void RegisterEvaluationHandler(std::string_view Opcode, Scratch::EvaluationHandler Handler);

    /**
     * Registers an opcode compiler.
     * @param Opcode The opcode to handle compilation for
     * @param Handler The function that compiles the opcode
     */
    void RegisterCompileHandler(std::string_view Opcode, Scratch::CompileHandler Handler);

    /**
     * Renders a sprite only if it's visible.
     * @param Sprite The sprite to render
     */
    void RenderSprite(Scratch::ScratchSprite & Sprite);

    /**
     * De-initializes SDL3.
     */
    void Quit(void);

    namespace Pseudo {
        bool IsPseudo(std::string ProcCode);
        Scratch::ScratchStatus Execute(std::string ProcCode, Scratch::ScratchBlock & Block);
        void RegisterAll(void);
    };
};
