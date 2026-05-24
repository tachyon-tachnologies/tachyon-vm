# Agent Guide for Tachyon VM

This repository implements a Scratch VM in C++ and SDL3. The codebase is a single executable built with CMake and depends on `SDL3` and `libzip` via the vendored `vcpkg` submodule.

## What matters most
- Build from the repository root.
- Do not use MSVC: the project explicitly rejects `MSVC` in `CMakeLists.txt`.
- Use a Clang/GCC toolchain compatible with C++23.
- The build disables RTTI and exceptions (`-fno-rtti`, `-fno-exceptions`). Avoid introducing code that requires them.
- The compiler flags also include `-Wextra`, `-Wpointer-arith`, `-Wcast-align`, `-Wredundant-decls`, `-Wformat`, and `-Wformat-security`.

## Build commands
1. Bootstrap vcpkg:
   - Windows: `./vcpkg/bootstrap-vcpkg.bat`
   - Unix: `./vcpkg/bootstrap-vcpkg.sh`
2. Install dependencies:
   - Windows: `vcpkg install sdl3:x64-windows libzip:x64-windows`
   - Linux/macOS: `vcpkg install sdl3:x64-linux libzip:x64-linux`
3. Configure and build with CMake from repo root.

## Key project structure
- `Source/Include/`: public headers used by the project.
- `Source/Libraries/`: third-party or helper library code.
- `Source/Scratch/`: Scratch language blocks, flow control, operators, and runtime scaffolding.
- `Source/TachyonCore/`: the core VM implementation, compiler, encoder, runtime, and main entrypoint.

## What to prioritize
- Keep changes local to the source tree unless modifying the build or dependencies.
- Preserve the current code style and low-level C++ optimizations.
- Prefer small, incremental improvements over broad refactors.

## Useful references
- Root README: `README.md`
- Build configuration: `CMakeLists.txt`

## Notes for AI agents
- If a change touches compilation or dependencies, verify it still configures with CMake and links SDL3/libzip.
- Preserve the explicit `-fno-rtti` / `-fno-exceptions` requirement unless the user asks to relax it.
- Avoid large API rewrites without user approval.
