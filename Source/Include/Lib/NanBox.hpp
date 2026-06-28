#pragma once

#include <string>
#include <cstdint>
#include <Common.hpp>

#define NANBOX_NORMAL_MASK  0x7FFC000000000000
// bit 0 is reserved
#define NANBOX_TRUE         (NANBOX_NORMAL_MASK | 3)
#define NANBOX_FALSE        (NANBOX_NORMAL_MASK | 2)

#define NANBOX_STRING_MASK  0x7FFD000000000000

namespace NanBox {
    using BoxedValue = uint64_t;
    BoxedValue __hot Box(double Number);
    BoxedValue __hot Box(bool Boolean);
    BoxedValue __hot Box(const std::string String);

    double __hot UnboxDouble(BoxedValue Value);
    std::string & __hot UnboxString(BoxedValue Value);
    bool __hot  UnboxBoolean(BoxedValue Value);

    /**
     * Converts the boxed value to a double regardless of the type
     * @param Value The boxed value
     * @returns The boxed value as a double
     */
    double __hot UnboxAsDouble(BoxedValue Value);

    /**
     * Converts the boxed value to a string regardless of the type
     * @param Value The boxed value
     * @returns The boxed value as a string
     */
    std::string __hot UnboxAsString(BoxedValue Value);

    /**
     * Converts the boxed value to a boolean regardless of the type
     * @param Value The boxed value
     * @returns The boxed value as a boolean
     */
    bool __hot UnboxAsBoolean(BoxedValue Value);

    static constexpr bool __hot IsDouble(BoxedValue Boxed) {
        return ((Boxed & NANBOX_NORMAL_MASK) != NANBOX_NORMAL_MASK);
    }

    static constexpr bool __hot  IsString(BoxedValue Boxed) {
        return ((Boxed & NANBOX_STRING_MASK) != NANBOX_STRING_MASK);
    }

    static constexpr bool __hot IsBoolean(BoxedValue Boxed) {
        return (((Boxed & NANBOX_NORMAL_MASK) == NANBOX_NORMAL_MASK) && (Boxed & (1 << 1)));
    }

    template <typename Type>
    bool __hot HoldsType(BoxedValue Value) {
        if constexpr (std::is_same_v<Type, double>) {
            if (IsDouble(Value)) {
                return true;
            }
        } else if constexpr (std::is_same_v<Type, std::string>) {
            if (IsString(Value)) {
                return true;
            }
        } else if constexpr (std::is_same_v<Type, bool>) {
            if (IsBoolean(Value)) {
                return true;
            }
        }
        return false;
    }
};