#pragma once

#include <string>
#include <cstdint>

namespace Tachyon {
    namespace GarbageCollector {
        /**
         * Adds a string to the garbage collector's string pool
         * @param String The string to add into the string pool
         * @returns An ID that points to the string in the string pool
         */
        uint32_t AddToStringPool(const std::string & String);
        
        /**
         * Fetchs a string from the string pool using the string's ID
         * @param StringId The string ID to fetch from the string pool
         * @returns The string in the string pool
         */
        std::string & GetFromStringPool(const uint32_t StringId);

        /**
         * Removes a string from the garbage collector's string pool
         * @param StringId The string ID to remove from the string pool
         */
        void RemoveFromStringPool(const uint32_t StringId);
    };
};