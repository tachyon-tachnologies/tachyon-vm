# Tachyon VM
Tachyon VM is a reimplementation of the Scratch VM written in C++ and SDL3. It aims to be the killer of any Scratch mod/runtime that currently exists.

# Compiling the VM
To compile the Tachyon VM, you first need to perform the following prerequisites:
1. Install Clang (for Windows systems, install it through MinGW and add it to your PATH). MSVC tools will **not** work.
2. Install the required libraries for Tachyon's VM

Let us start with installing the required libraries for Tachyon's VM.

## Prerequisites
Before you start, please install ninja. If you're on Windows, please run the following command ``winget install -e --id Ninja-build.Ninja``

## Installing necessary libraries
Tachyon's VM requires SDL3, and libzip to be installed in order to compile successfully. To install the libraries, follow the directions below:

1. Clone the repository along with it's submodules.
2. Run ``.\vcpkg\bootstrap-vcpkg.bat`` for Windows systems, or ``./vcpkg/bootstrap-vcpkg.sh`` for Linux systems to install the package manager required to install the project's libraries
3. Then, run ``vcpkg integrate install`` to be able to install libraries system-wide.
4. Now, you can install the three dependencies: ``vcpkg install sdl3 libzip simdjson --triplet=x64-mingw-dynamic --host-triplet=x64-mingw-dynamic`` for Windows systems, or ``vcpkg install sdl3 libzip simdjson --triplet=x64-linux --host-triplet=x64-linux`` for Linux systems.

## Building the VM
Now that you've got that done, you may now follow these last two easy steps:

1. Run ``cmake -B build`` to generate build files for Tachyon.
2. And finally, run ``cmake --build build`` for UNIX-esque systems to build Tachyon.

Congrats, you just built Tachyon.

# Making clangd work with Tachyon
If you're having an issue with getting includes working properly while using a code editor like neovim to develop Tachyon, please execute the following command after building: ``ln -s build/compile_commands.json compile_commands.json``
