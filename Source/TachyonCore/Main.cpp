#include <Common.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Debug.hpp>
#include <Tachyon/Assembler.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Scheduler.hpp>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int main(int argc, char * argv[]) {
    std::cout << "Tachyon: the faster replacement for TurboWarp" << std::endl;
    /* check for potential file */
    if (argc < 2) {
        std::cout << "Please give a sb3 project to run." << std::endl;
        return -1;
    }
    /* verify that the project exists, and parse the project */
    Scratch::ScratchProject MainProject = Scratch::ScratchProject(argv[1]);
    if (MainProject.IsLoaded() == false) {
        std::cout << "Failed to open " << argv[1] << std::endl;
        return -1;
    }
    /* initialize tachyon */
    Tachyon::Init();
    if (MainProject.ParseContents() < 0) {
        std::cout << "Failed to parse project contents" << std::endl;
        return -1;
    }
    /* initialize sdl3 */
    if (Tachyon::InitWindow() == false) {
        std::cout << SDL_GetError() << std::endl;
        return -1;
    }
    /* now we can do something */
    Tachyon::InitializeScheduler(MainProject);
    DebugInfo("Beginning execution..\n");
    Tachyon::MainLoop();
    Tachyon::Quit();
    return 0;
}
