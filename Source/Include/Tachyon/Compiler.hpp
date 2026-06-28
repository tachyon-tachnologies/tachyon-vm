#pragma once

#include <Tachyon/Debug.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Assembler.hpp>
#include <Common.hpp>
#include <string>

#define CompilerAssert(Condition, ...) \
    TachyonAssertMsg(Condition, "Cannot compile script: " __VA_ARGS__);

namespace Tachyon {
    void __hot Compile(Scratch::ScratchScript & Script, std::string BlockId);
    void __hot CompileProcedure(Scratch::ScratchProcedure & Procedure, Scratch::ScratchScript & Script, std::string BlockId);
};