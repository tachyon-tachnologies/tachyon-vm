#pragma once

#include <Lib/NanBox.hpp>

namespace Scratch {
    extern "C" void RuntimeSay(const NanBox::BoxedValue Value);

    namespace Looks {
        void RegisterAll(void);
    };
};
