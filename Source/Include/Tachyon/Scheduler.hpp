#pragma once

#include <Scratch/Scripts.hpp>
#include <Scratch/Common.hpp>

namespace Tachyon {
    /**
     * Initializes the scheduler.
     * @param Project The scratch project to execute.
     */
    void InitializeScheduler(Scratch::ScratchProject & Project);

    /**
     * Gets the currently running script.
     * @return The script that is currently running.
     */
    Scratch::ScratchScript * GetCurrentScript(void);

    /**
     * Adds a script to the ready queue of the scheduler.
     * @param Script The script
     */
    void ScriptAddReadyQueue(Scratch::ScratchScript & Script);
};