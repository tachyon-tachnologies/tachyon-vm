#pragma once

#include <Scratch/Procedures.hpp>
#include <Tachyon/Encoder.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Common.hpp>
#include <string_view>
#include <cstdint>

#include <SDL3/SDL_render.h>

/**
 * Configuration option: Script watchdog.
 * Enables a watchdog for all scripts.
 */
#define TACHYON_CFG_WATCHDOG  (1 << 0)

/**
 * Configuration option: Pseudo-block support.
 * When enabled, procedures with certain names and structure can be called to make the VM do heavy tasks instead of the project.
 */
#define TACHYON_CFG_PBLOCK    (1 << 1)

/**
 * Configuration option: Shit talk.
 * Talks shit about WarpDrive (build a x86 JIT compiler before talking shit, loser), TurboWarp (we don't mean it for yall), and any other Scratch mod that is slower than us.
 */
#define TACHYON_CFG_SHITTALK  (1 << 2)

/**
 * Configuration option: Blitz
 * Makes the VM execute code extremely fast.
 */
#define TACHYON_CFG_BLITZ     (1 << 3)

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
     * Initializes SDL3.
     */
    int Init(void);

    /**
     * Initializes the scheduler.
     * @param The scratch project to execute.
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
     * @param The script
     */
    void ScriptAddReadyQueue(Scratch::ScratchScript Script);

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
     * Gets the stage sprite.
     * IMPORTANT: Must be called AFTER Tachyon::InitializeScheduler()
     * @return The stage sprite.
     */
    Scratch::ScratchSprite * GetStage(void);

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
     * Registers an opcode handler.
     * @param Opcode string.
     * @param The function that handles the specific opcode.
     */
    void RegisterOpHandler(std::string_view Opcode, Scratch::OpcodeHandler Handler);

    /**
     * Registers an evaluation (reporter) handler.
     * @param Opcode string.
     * @param The function that handles the specific opcode.
     */
    void RegisterEvaluationHandler(std::string_view Opcode, Scratch::EvaluationHandler Handler);

    /**
     * Renders a sprite only if it's visible.
     * @param The sprite to render
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
