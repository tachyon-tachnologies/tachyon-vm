# Tachyon VM
Tachyon VM is a reimplementation of the Scratch VM written in C++ and SDL3. It aims to be the killer of any Scratch mod/runtime that currently exists.

# Compiling the VM
To compile the Tachyon VM, you first need to perform the following prerequisites:
1. Install Clang (for Windows systems, install it through MinGW and add it to your PATH). MSVC tools will **not** work.
2. Install the required libraries for Tachyon's VM

Let us start with installing the required libraries for Tachyon's VM.

## Installing necessary libraries
Tachyon's VM requires SDL3, and libzip to be installed in order to compile successfully. To install the libraries, follow the directions below:

1. Clone the repository along with it's submodules.
2. Run ``.\vcpkg\bootstrap-vcpkg.bat`` for Windows systems, or ``./vcpkg/bootstrap-vcpkg.sh`` for UNIX-esque systems to install the package manager required to install the project's libraries
3. Then, run ``vcpkg integrate install`` to be able to install libraries system-wide.
4. Now, you can install the two dependencies: ``vcpkg install sdl3:x64-windows libzip:x64-windows`` for Windows systems, or ``vcpkg install sdl3:x64-linux libzip:x64-linux`` for UNIX-esque systems.

Now that you've got that done, you may now run ``cmake`` to build the Tachyon's VM.