#include <Tachyon/GC.hpp>
#include <vector>

using namespace Tachyon;

static std::vector<std::string> StringPool;

size_t GarbageCollector::GetNumStrings(void) {
    return StringPool.size();
}

uint32_t GarbageCollector::AddToStringPool(const std::string & String) {
    StringPool.emplace_back(String);
    return static_cast<uint32_t>(StringPool.size() - 1);
}

std::string & GarbageCollector::GetFromStringPool(const uint32_t StringId) {
    return StringPool.at(StringId);
}

void GarbageCollector::RemoveFromStringPool(const uint32_t StringId) {
    StringPool.erase(StringPool.begin() + StringId);
}