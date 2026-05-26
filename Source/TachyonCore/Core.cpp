#include <SDL3/SDL.h>
#include <Scratch/Common.hpp>
#include <Scratch/ControlFlow.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/Looks.hpp>
#include <Scratch/Motion.hpp>
#include <Scratch/Operator.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Reporters.hpp>
#include <Scratch/Sensing.hpp>
#include <Tachyon/Events.hpp>
#include <Tachyon/Tachyon.hpp>

static Tachyon::Tachyon_VirtualMachine VM;
static bool TachyonInitialized = false;

int Tachyon::Init(void) {
    if (TachyonInitialized == true) {
        return 0;
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        return -1;
    }
    VM.TachyonWindow = SDL_CreateWindow("Tachyon Virtual Machine", 480, 360, 0);
    if (VM.TachyonWindow == nullptr) {
        SDL_Quit();
        return -1;
    }
    VM.TachyonRenderer = SDL_CreateRenderer(VM.TachyonWindow, nullptr);
    if (VM.TachyonRenderer == nullptr) {
        SDL_DestroyWindow(VM.TachyonWindow);
        SDL_Quit();
        return -1;
    }
    SDL_SetRenderVSync(VM.TachyonRenderer, SDL_RENDERER_VSYNC_ADAPTIVE);
    Scratch::Motion::RegisterAll();
    Scratch::Operator::RegisterAll();
    Scratch::Procedures::RegisterAll();
    Scratch::Data::RegisterAll();
    Scratch::Looks::RegisterAll();
    Scratch::ControlFlow::RegisterAll();
    Scratch::Sensing::RegisterAll();
    Scratch::Reporters::RegisterAll();
    Scratch::Events::RegisterAll();
    Tachyon::Pseudo::RegisterAll();
    /* EXPERIMENTAL FEATURE */
    VM.Configuration = (TACHYON_CFG_PBLOCK | TACHYON_CFG_SHITTALK);
    TachyonInitialized = true;
    return 0;
}

Tachyon::Tachyon_VirtualMachine * Tachyon::GetVM(void) {
    return &VM;
}

Scratch::ScratchProject * Tachyon::GetLoadedProject(void) {
    return VM.Project;
}

Tachyon::TachyonConfig __hot Tachyon::GetConfigVM(void) {
    return VM.Configuration;
}

void __hot Tachyon::Render(void) {
    SDL_RenderClear(VM.TachyonRenderer);
    SDL_SetRenderDrawColor(VM.TachyonRenderer, 255, 255, 255, 0);
    SDL_RenderPresent(VM.TachyonRenderer);
    return;
}

void __hot Tachyon::MainLoop(void) {
    while(VM.ShouldExit == false) {
        SDL_Event event;
        while(SDL_PollEvent(&event) == true) {
            switch (event.type) {
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) {
                        VM.ShouldExit = true;
                        return;
                    }
                    break;
                case SDL_EVENT_QUIT:
                    VM.ShouldExit = true;
                    return;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    VM.ShouldExit = true;
                    return;
                default:
                    break;
            }
        }
        VM.ShouldExit = Tachyon::Step();
        /* only render if anything has been rendered */
        Tachyon::Render();
    }
}

void Tachyon::Quit(void) {
    if (VM.TachyonWindow)
        SDL_DestroyWindow(VM.TachyonWindow);
    if (VM.TachyonRenderer)
        SDL_DestroyRenderer(VM.TachyonRenderer);
    SDL_Quit();
}
