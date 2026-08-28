#include <bit>
#include <iostream>
#include <string>
#include <type_traits>
#include <Tachyon/Debug.hpp>
#include <Tachyon/GC.hpp>
#include <Lib/NanBox.hpp>
#include <Common.hpp>

using namespace Tachyon;
using namespace NanBox;

BoxedValue __hot NanBox::Box(double Number) {
    BoxedValue Boxed = std::bit_cast<BoxedValue>(Number);
    // DebugInfo("%0.0f -> 0x%016zx...\n", Number, Boxed);
    return Boxed;
}

BoxedValue __hot NanBox::Box(bool Boolean) {
    BoxedValue Boxed = Boolean ? NANBOX_TRUE : NANBOX_FALSE;
    // DebugInfo("%s -> 0x%016zx...\n", Boolean ? "true" : "false", Boxed);
    return Boxed;
}

BoxedValue __hot NanBox::Box(const std::string String) {
    uint32_t StringId = GarbageCollector::AddToStringPool(String);
    BoxedValue Boxed = (NANBOX_STRING_MASK | StringId);
    // DebugInfo("\"%s\" -> 0x%016zx...\n", String.c_str(), Boxed);
    return Boxed;
}

double __hot NanBox::UnboxDouble(BoxedValue Value) {
    TachyonAssertMsg(IsDouble(Value), "Boxed value is NOT a double!!\n");
    return std::bit_cast<double>(Value);
}

std::string & __hot NanBox::UnboxString(BoxedValue Value) {
    TachyonAssertMsg(IsString(Value), "Boxed value is NOT a string!!\n");
    uint32_t StringId = (Value & 0xFFFFFFFF);
    return GarbageCollector::GetFromStringPool(StringId);
}

bool __hot NanBox::UnboxBoolean(BoxedValue Value) {
    TachyonAssertMsg(IsBoolean(Value), "Boxed value is NOT a boolean!!\n");
    return static_cast<bool>(Value & 1);
}

double __hot NanBox::UnboxAsDouble(BoxedValue Value) {
    if (IsDouble(Value)) {
        return std::bit_cast<double>(Value);
    }
    if (IsBoolean(Value)) {
        return static_cast<double>(Value & 1);
    }
    return 0.0;
}

std::string __hot NanBox::UnboxAsString(BoxedValue Value) {
    if (IsString(Value)) {
        uint32_t StringId = (Value & 0xFFFFFFFF);
        return GarbageCollector::GetFromStringPool(StringId);
    }
    if (IsBoolean(Value)) {
        return (Value & 1) ? "true" : "false";
    }
    // TODO: remove fixed buffer
    char Buffer[64];
    std::to_chars_result Result = std::to_chars(Buffer, Buffer + sizeof(Buffer), std::bit_cast<double>(Value), std::chars_format::fixed);
    return std::string(Buffer, Result.ptr);
}

bool __hot NanBox::UnboxAsBoolean(BoxedValue Value) {
    if (IsBoolean(Value)) {
        return static_cast<bool>(Value & 1);
    }
    if (IsDouble(Value)) {
        return static_cast<bool>(std::bit_cast<double>(Value) == true);
    }
    uint32_t StringId = (Value & 0xFFFFFFFF);
    std::string_view String = GarbageCollector::GetFromStringPool(StringId);
    return (String == "true");
}
