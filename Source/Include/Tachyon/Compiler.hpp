#pragma once

#include <string>

#include <Tachyon/Debug.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/Scripts.hpp>
#include <Tachyon/Assembler.hpp>
#include <Tachyon/TinyIR.hpp>
#include <Lib/NanBox.hpp>
#include <Common.hpp>

#define CompilerAssert(Condition, ...) \
    TachyonAssertMsg(Condition, "Cannot compile script: " __VA_ARGS__)

namespace Tachyon {
    OutputCodeInfo __hot Compile(Scratch::ScratchScript & Script, std::string BlockId);
};
