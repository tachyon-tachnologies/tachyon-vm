#pragma once

#include <Tachyon/Debug.hpp>
#include <Tachyon/GC.hpp>
#include <Lib/NanBox.hpp>
#include <Common.hpp>
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <string>
#include <variant>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include <Lib/SIMDJson.h>
#pragma GCC diagnostic pop

using namespace NanBox;
using namespace simdjson;

#define IS_VALID_BASE10(c) ((c) >= '0' && (c) <= '9' || (c) == '-' || (c) == '+' || (c) == 'E' || (c) == 'e' || (c) == '.')
#define IS_INVALID_BASE10(c) ((c) == '-' || (c) == '+' || (c) == 'E' || (c) == 'e')

/* scratch uses 53-bit precision so doubles are perfect */
namespace Scratch {
    static BoxedValue __hot StringToNum(std::string_view String) {
        /* make a string snapshot of the string_view just so we dont lose the og value */
        std::string StringSnapshot = std::string(String);

        if (unlikely(String.empty() == true)) {
            return Box(StringSnapshot);
        }
        /* remove whitespace in front */
        while(String[0] == ' ' && String.size() > 1) String.remove_prefix(1);

        if (unlikely(String.empty() == true)) {
            return Box(StringSnapshot);
        }
        /* remove whitespace in back */
        while(String[String.length() - 1] == ' ' && String.size() > 1) String.remove_suffix(1);

        if (String == "Infinity" || String == "+Infinity") {
            return Box(std::numeric_limits<double>::infinity());
        } else if (String == "-Infinity") {
            return Box(-std::numeric_limits<double>::infinity());
        }
        uint8_t RadixModifier = 10;

        if (String.length() < 2) {
            goto SkipChecks;
        }

        if (String[0] == '0') {
            switch(String[1]) {
                case 'X':
                case 'x':
                    RadixModifier = 16;
                    break;
                case 'O':
                case 'o':
                    RadixModifier = 8;
                    break;
                case 'B':
                case 'b':
                    RadixModifier = 2;
                    break;
                default:
                    break;
            }
            if (RadixModifier != 10) {
                String.remove_prefix(2);
                uint64_t NonDecConversion;
                std::from_chars_result Result = std::from_chars(String.begin(), String.end(), NonDecConversion, RadixModifier);
                if (Result.ec == std::errc::invalid_argument) {
                    /* bad num */
                    return Box(StringSnapshot);
                } else if (Result.ec == std::errc::result_out_of_range) {
                    /* other possible result could be out of range (infinity for scratch) */
                    return Box(std::numeric_limits<double>::infinity());
                }
                return Box(static_cast<double>(NonDecConversion));
            }
        }
SkipChecks:
        if (IS_INVALID_BASE10(String[String.length() - 1]) == true) {
            return Box(StringSnapshot);
        }
        bool PastEuler = false;
        for(size_t i = 0; i < String.length(); i++) {
            const char c = String[i];
            if (IS_VALID_BASE10(c) == false) {
                return Box(StringSnapshot);
            }
            if (PastEuler) {
                if (c == '.') {
                    return Box(StringSnapshot);
                }
            }
            if (c == 'e' || c == 'E') {
                PastEuler = true;
            }
        }
        double ConvertedBase10;
        std::from_chars_result Result = std::from_chars(String.begin(), String.end(), ConvertedBase10);
        if (Result.ec == std::errc::invalid_argument) {
            return Box(StringSnapshot);
        } else if (Result.ec == std::errc::result_out_of_range) {
            if (String[0] == '-') {
                return Box(-std::numeric_limits<double>::infinity());
            } else {
                return Box(std::numeric_limits<double>::infinity());
            }
        }
        /* could be nan or infinity */
        if (unlikely(std::isnan(ConvertedBase10) == true)) {
            return Box(std::numeric_limits<double>::quiet_NaN());
        }
        return Box(ConvertedBase10);
    }

    static BoxedValue __hot SanitizeData(ondemand::value VariableData) {
        simdjson::ondemand::json_type ValueType;
        TachyonAssert(VariableData.type().get(ValueType) == error_code::SUCCESS);
        switch(ValueType) {
            case ondemand::json_type::string: {
                /* not too reliable to detect strings. could be hex, octal, binary, or a number. */
                std::string String;
                TachyonAssert(VariableData.get_string().get(String) == error_code::SUCCESS);
                /* TODO: If error, it returns the string without modifications. */
                return StringToNum(String);
            }
            case ondemand::json_type::boolean: {
                bool SanitizedBool;
                TachyonAssert(VariableData.get_bool().get(SanitizedBool) == error_code::SUCCESS);
                return Box(SanitizedBool);
            }
            case ondemand::json_type::number: {
                /* various number types */
                simdjson::ondemand::number_type NumberType;
                TachyonAssert(VariableData.get_number_type().get(NumberType) == error_code::SUCCESS);
                switch(NumberType) {
                    case ondemand::number_type::big_integer: {
                        return Box(std::numeric_limits<double>::infinity());
                    }
                    default: {
                        double SanitizedNum;
                        TachyonAssert(VariableData.get_double().get(SanitizedNum) == error_code::SUCCESS);
                        return Box(SanitizedNum);
                    }
                }
            }
            default: {
                TachyonAbort("There should be no other types than a bool, string, or number!!\n");
            }
        }
    }

    class ScratchVariable_Base {
        public:
            constexpr std::string & GetName(void) {
                return this->Name;
            }
            constexpr bool IsPublic(void) {
                return this->Public;
            }
        protected:
            std::string Name;
            bool Public;
    };

    class ScratchVariable : public ScratchVariable_Base {
        public:
            explicit ScratchVariable(ondemand::array VariableData, const bool IsPublic) {
                /* VariableData[0] = list name, VariableData[1] = actual data */
                simdjson::simdjson_result Result = VariableData.at(0);
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Name) == error_code::SUCCESS);

                VariableData.reset();
                Result = VariableData.at(1);

                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::value RawValue;
                TachyonAssert(Result.get(RawValue) == error_code::SUCCESS);
                this->Data = SanitizeData(RawValue);

                VariableData.reset();
                this->Public = IsPublic;
            }

            void __hot SetData(BoxedValue && NewData) {
                this->Data = NewData;
                NewData = 0;
            }
            constexpr BoxedValue __hot GetData(void) const {
                return Data;
            }
        private:
            BoxedValue Data;
    };

    class ScratchList : public ScratchVariable_Base {
        public:
            explicit ScratchList (ondemand::array ListData, const bool IsPublic) {
                /* same applies for ScratchList as it does for ScratchVariable; 
                 * ListData[0] = list name, ListData[1] = [actual data] */
                
                simdjson::simdjson_result Result = ListData.at(0);
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Name) == error_code::SUCCESS);
                /* prepare for lazy loading */
                ListData.reset();
                Result = ListData.at(1);

                ondemand::array ListArray;
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_array().get(ListArray) == error_code::SUCCESS);
                TachyonAssert(ListArray.count_elements().get(this->TotalItems) == error_code::SUCCESS);

                std::string RawJsonString;
                TachyonAssert(ListArray.raw_json().get(RawJsonString) == error_code::SUCCESS);
                this->ListJson = padded_string(RawJsonString);

                ListData.reset();

                if (this->TotalItems > 200000) {
                    DebugWarn("List \"%s\" goes over 200,000 items; memory usage is bound to increase. To reduce memory usage, consider using pseudo-blocks to create a less memory-expensive buffer (if the list consists of number values under 256).\n", this->Name.c_str());
                    DebugWarn("Estimated memory usage without buffer: %d bytes\n", sizeof(BoxedValue) * this->TotalItems);
                }
                this->LazyLoad = true;
                this->Public = IsPublic;
                this->Size = this->TotalItems;
            }

            void __hot SwitchToBuffer(void) {
                if (unlikely(std::holds_alternative<uint8_t *>(this->Elements) == true)) {
                    /* you're already a buffer pal */
                    return;
                }

                if (unlikely(this->TotalItems == 0)) {
                    DebugError("Please use this function on a populated list.\n");
                    return;
                }

                uint8_t * NewBuffer = new (std::nothrow) uint8_t[this->TotalItems];

                if (unlikely(NewBuffer == nullptr)) {
                    DebugError("Failed to allocate UINT8 buffer for \"%s\"\n", this->Name.c_str());
                    return;
                }

                std::vector<BoxedValue> & Cache = std::get<std::vector<BoxedValue>>(this->Elements);

                ondemand::parser JsonParser;
                simdjson::simdjson_result DataDoc = JsonParser.iterate(this->ListJson);
                TachyonAssert(DataDoc.error() == error_code::SUCCESS);
                ondemand::array DataArray;
                TachyonAssert(DataDoc.get_array().get(DataArray) == error_code::SUCCESS);

                size_t i = 0;
                for(auto Value : DataArray) {
                    ondemand::value RawData;
                    TachyonAssert(Value.get(RawData) == error_code::SUCCESS);
                
                    BoxedValue Item = SanitizeData(RawData);

                    if (unlikely(HoldsType<double>(Item) == false || UnboxDouble(Item) > 255)) {
                        DebugError("Failed to load buffer \"%s\": The list NEEDS to only have numbers, and they can't be greater than 255.\n", this->Name.c_str());
                        delete[] NewBuffer;
                        return;
                    }

                    uint8_t Byte(UnboxDouble(Item));
                    NewBuffer[i] = Byte;
                    i++;
                }
                /* bye bye vector */
                Cache.clear();
                this->Elements = NewBuffer;
                this->LazyLoad = false;
                DebugInfo("Buffer \"%s\" is ready for use.\n", this->Name.c_str());
            }

            void __hot ClearElements(void) {
                if (auto VectorPtr = std::get_if<std::vector<BoxedValue>>(&this->Elements)) {
                    std::vector<BoxedValue> & ElementVector = *VectorPtr;
                    ElementVector.clear();
                    this->TotalItems = 0;
                    this->LazyLoad = false;
                    return;
                }
                uint8_t * Buffer = std::get<uint8_t *>(this->Elements);
                memset(Buffer, 0, this->Size);
                this->TotalItems = 0;
            }

            BoxedValue __hot Get(const size_t Index) {
                if (unlikely(Index >= this->TotalItems)) {
                    return Box("");
                }
                if (auto VectorPtr = std::get_if<std::vector<BoxedValue>>(&this->Elements)) {
                    std::vector<BoxedValue> & ListVector = *VectorPtr;
                    if (this->LazyLoad == true) {
                        if (likely(Index < ListVector.size())) {
                            /* cache HIT */
                            //DebugInfo("CACHE HIT\n");
                            return ListVector.at(Index);
                        }
                        /* cache miss */
                        ondemand::parser JsonParser;
                        auto DataArray = JsonParser.iterate(this->ListJson);
                        TachyonAssert(DataArray.error() == error_code::SUCCESS);

                        ondemand::value RawData;
                        TachyonAssert(DataArray.at(Index).get(RawData) == error_code::SUCCESS);

                        BoxedValue Data = SanitizeData(RawData);
                        ListVector.resize(Index + 1);
                        ListVector.insert(ListVector.begin() + Index, Data);
                        //DebugInfo("CACHE MISS\n");
                        return Data;
                    }
                    /* lazy load off = all items loaded in the vector */
                    return ListVector.at(Index);
                } else {
                    uint8_t * Buffer = std::get<uint8_t *>(this->Elements);
                    return BoxedValue(static_cast<double>(Buffer[Index]));
                }
                __unreachable;
            }

            void __hot Set(const BoxedValue && Data, const size_t Index) {
                if (unlikely(Index >= this->TotalItems)) {
                    return;
                }
                /* regular list */
                if (auto VectorPtr = std::get_if<std::vector<BoxedValue>>(&this->Elements)) {
                    std::vector<BoxedValue> & ListVector = *VectorPtr;
                    if (this->LazyLoad == true) {
                        if (likely(Index < ListVector.size())) {
                            ListVector[Index] = Data;
                            return;
                        }
                        ListVector.resize(Index);
                        ListVector.insert(ListVector.begin() + Index, Data);
                        return;
                    }
                    ListVector[Index] = Data;
                    return;
                }
                uint8_t * Buffer = std::get<uint8_t *>(this->Elements);
                if (unlikely(HoldsType<double>(Data) == false || UnboxDouble(Data) > 255)) {
                    DebugWarn("If you're going to write data into the buffer, please write a number value under 256, otherwise nothing will be written to the buffer.\n");
                }
                // TODO: NanBox unboxing.
                Buffer[Index] = static_cast<uint8_t>(UnboxDouble(Data));
            }

            void __hot Append(const BoxedValue && Data) {
                if (auto VectorPtr = std::get_if<std::vector<BoxedValue>>(&this->Elements)) {
                    /* lazy loading is no longer useful */
                    std::vector<BoxedValue> & ListVector = *VectorPtr;

                    if (this->LazyLoad == true) {
                        this->TotalItems++;

                        ListVector.clear();
                        ListVector.resize(this->TotalItems);

                        ondemand::parser JsonParser;
                        simdjson::simdjson_result DataDoc = JsonParser.iterate(this->ListJson);
                        TachyonAssert(DataDoc.error() == error_code::SUCCESS);
                        ondemand::array DataArray;
                        TachyonAssert(DataDoc.get_array().get(DataArray) == error_code::SUCCESS);

                        size_t i = 0;
                        for(auto Value : DataArray) {
                            ondemand::value RawData;
                            TachyonAssert(Value.get(RawData) == error_code::SUCCESS);
                        
                            BoxedValue Item = SanitizeData(RawData);

                            ListVector.insert(ListVector.begin() + i, Item);
                            i++;
                        }
                        ListVector.insert(ListVector.begin() + this->TotalItems - 1, Data);
                        this->LazyLoad = false;
                        return;
                    }
                    this->TotalItems++;
                    if (this->TotalItems > this->Size) {
                        this->Size++;
                    }
                    ListVector.resize(this->TotalItems);
                    ListVector.insert(ListVector.begin() + this->TotalItems - 1, Data);
                    return;
                }
                DebugWarn("Cannot append items to buffer.\n");
            }
            size_t TotalItems = 0;
            size_t Size = 0;
        private:
            std::variant<std::vector<BoxedValue>, uint8_t *> Elements;
            padded_string ListJson;
            bool LazyLoad;
    };

    namespace Data {
        void RegisterAll(void);
    };
};
