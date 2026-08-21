#pragma once

#include <string>
#include <cstdint>
#include <variant>

#include <Scratch/Data.hpp>
#include <simdjson.h>
#include <Lib/NanBox.hpp>

using namespace NanBox;

namespace Scratch {
    class ScratchSprite;
    class ScratchBlock;
    /*
     * Field object
     */

    enum class FieldType : uint8_t { VariableField, ListField, StringField, BroadcastOption, InvalidField };

    /**
     * Scratch field descriptor
     */
    class ScratchField {
        public:
            constexpr bool IsType(FieldType Type) const {
                return (this->Type == Type);
            }

            ScratchField(ScratchBlock & Owner, const std::string & Key, simdjson::ondemand::array & FieldObject);

            std::variant<ScratchList *, ScratchVariable *, std::string> Field;
        private:
            void ParseDataField(simdjson::ondemand::array & FieldObject, ScratchBlock & Block);
            void ParseBroadcastField(simdjson::ondemand::array & FieldObject);
            void ParseStringOption(simdjson::ondemand::array & FieldObject);
            FieldType Type;
    };

    /*
     * Input object
     */

    enum class ScratchShadow : uint8_t {
        INPUT_SAME_BLOCK_SHADOW = 1,
        INPUT_BLOCK_NO_SHADOW,
        INPUT_DIFF_BLOCK_SHADOW
    };

    enum class ScratchPrimitive : uint8_t {
        INPUT_MATH_NUM = 4,
        INPUT_POSITIVE_NUM,
        INPUT_WHOLE_NUM,
        INPUT_INTEGER_NUM,
        INPUT_ANGLE_NUM,
        INPUT_COLOR_PICKER,
        INPUT_TEXT,
        INPUT_BROADCAST,
        INPUT_VAR,
        INPUT_LIST
    };

    /* value input */
    struct Input_Value {
        std::variant<BoxedValue, ScratchList *, ScratchVariable *, std::string> Value;
        ScratchPrimitive PrimitiveType;
    };

    /* operand input */
    struct Input_Operand {
        BoxedValue OperandValue;
        ScratchPrimitive PrimitiveType;
    };

    enum class InputType : uint8_t { ConditionInput, SubstackInput, ProcedureDefinition, ValueInput, BroadcastInput, InvalidInput };

    /**
     * Scratch input descriptor
     */
    class ScratchInput {
        public:
            constexpr bool IsType(InputType Type) const {
                return this->Type == Type;
            }

            constexpr InputType GetType(void) const {
                return this->Type;
            }

            constexpr bool IsReporter(void) const {
                return this->Reporter;
            }

            constexpr ScratchBlock * GetReporterBlock(void) {
                return this->ReporterBlock;
            }

            ScratchInput(ScratchBlock & Owner, const std::string & Key, simdjson::ondemand::array & InputObject);

            std::variant<Input_Value, std::string> Input;
        private:
            void ParseValueInput(ScratchSprite & Owner, simdjson::ondemand::array & InputObject);
            void ParseProcedureDefinition(simdjson::ondemand::array & InputObject);
            void ParseControlInput(simdjson::ondemand::array & InputObject);

            ScratchBlock * ReporterBlock = nullptr;
            InputType Type;
            ScratchShadow ShadowType;
            bool Reporter;
    };
};
