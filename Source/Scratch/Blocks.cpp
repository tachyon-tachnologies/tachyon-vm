#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Lib/SIMDJson.h>
#include <Common.hpp>
#include <unordered_set>

using namespace simdjson;
using namespace Scratch;

ScratchData __hot ScratchBlock::GetInputData(size_t InputNum) {
    /* bad InputNum */
    if (unlikely((InputNum > this->Inputs.size() - 1) || this->Inputs.empty())) {
        return ScratchData(double(0));
    }
    ScratchInput & Input = this->Inputs[InputNum];
    if (Input.HasReporter == true) {
        /* get it from the connected block instead */
        ScratchSprite & OwnerSprite = this->Sprite.get();
        ScratchBlock * ReporterBlock = OwnerSprite.GetBlockFromId(Input.ReporterKey);
        TachyonAssert(ReporterBlock != nullptr);
        ScratchData ReporterResult = ReporterBlock->Evaluate();
        return ReporterResult;
    }
    /* a normal input value */
    if (Input.Type == ScratchInput::InputType::ValueInput) {
        Input_Value InputValue = std::get<Input_Value>(Input.Input);
        ScratchSprite & OwnerSprite = this->GetOwnerSprite();
        ScratchData Data;
        if (std::holds_alternative<Field_Variable>(InputValue.Value)) {
            Field_Variable VariableField = std::get<Field_Variable>(InputValue.Value);
            if (VariableField.IsList == false) {
                ScratchVariable * Variable = OwnerSprite.GetVariableFromKey(VariableField.VariableKey);

                TachyonAssert(Variable != nullptr);

                Data = Variable->GetData();
            } else {
                ScratchList * List = OwnerSprite.GetListFromKey(VariableField.VariableKey);

                TachyonAssert(List != nullptr);

                /* UNIMPLEMENTED */
                Data = ScratchData(double(0));
                DebugError("Unimplemented #1\n");
            }

        } else if (std::holds_alternative<ScratchData>(InputValue.Value)) {
            Data = std::get<ScratchData>(InputValue.Value);
        }
        return Data;
    }
    return ScratchData(double(0));
}

ScratchInput __hot ScratchBlock::GetInput(size_t InputNum) {
    if (unlikely((InputNum > this->Inputs.size() - 1) || this->Inputs.empty())) {
        struct ScratchInput Input;
        Input.Type = ScratchInput::InputType::InvalidInput;
        return Input;
    }
    return this->Inputs[InputNum];
}

ScratchField __hot ScratchBlock::GetField(size_t FieldNum) {
    if (unlikely((FieldNum > this->Fields.size() - 1) || this->Fields.empty())) {
        struct ScratchField Field;
        Field.Type = ScratchField::FieldType::InvalidField;
        return Field;
    }
    return this->Fields[FieldNum];
}

ScratchSprite & __hot ScratchBlock::GetOwnerSprite(void) {
    return this->Sprite.get();
}

static inline ScratchShadow ParseShadowType(ondemand::array & InputObject) {
    InputObject.reset();

    simdjson::simdjson_result Result = InputObject.at(0);
    TachyonAssert(Result.error() == error_code::SUCCESS);
    uint64_t RawShadow;
    TachyonAssert(Result.get_uint64().get(RawShadow) == error_code::SUCCESS);

    InputObject.reset();
    return ScratchShadow(RawShadow);
}

static ScratchPrimitive ParsePrimitiveType(ondemand::array & InputObject) {
    InputObject.reset();

    simdjson::simdjson_result Result = InputObject.at(0);
    TachyonAssert(Result.error() == error_code::SUCCESS);
    uint64_t RawPrimitive;
    TachyonAssert(Result.get_uint64().get(RawPrimitive) == error_code::SUCCESS);

    InputObject.reset();

    return ScratchPrimitive(RawPrimitive & 0xFF);
}

static std::variant<Input_Value, std::string> SetupInputValue(ondemand::array & RawArray) {
    ScratchPrimitive PrimitiveType = ParsePrimitiveType(RawArray);

    switch(PrimitiveType) {
        case ScratchPrimitive::INPUT_VAR: {
            Input_Value Value;
            Field_Variable Variable;

            simdjson::simdjson_result Result = RawArray.at(1);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            TachyonAssert(Result.get_string().get(Variable.VariableName) == error_code::SUCCESS);

            RawArray.reset();

            Result = RawArray.at(2);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            TachyonAssert(Result.get_string().get(Variable.VariableKey) == error_code::SUCCESS);
            
            RawArray.reset();

            Variable.IsList = false;

            Value.PrimitiveType = PrimitiveType;
            Value.Value = Variable;

            RawArray.reset();

            return Value;
        }
        case ScratchPrimitive::INPUT_LIST: {
            Input_Value Value;
            Field_Variable Variable;

            simdjson::simdjson_result Result = RawArray.at(1);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            TachyonAssert(Result.get_string().get(Variable.VariableName) == error_code::SUCCESS);

            RawArray.reset();

            Result = RawArray.at(2);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            TachyonAssert(Result.get_string().get(Variable.VariableKey) == error_code::SUCCESS);

            RawArray.reset();

            Variable.IsList = true;

            Value.PrimitiveType = PrimitiveType;
            Value.Value = Variable;

            RawArray.reset();

            return Value;
        }
        case ScratchPrimitive::INPUT_BROADCAST: {
            std::string BroadcastKey;

            simdjson::simdjson_result Result = RawArray.at(2);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            TachyonAssert(Result.get_string().get(BroadcastKey) == error_code::SUCCESS);

            RawArray.reset();

            return BroadcastKey;
        }
        default: {
            Input_Value Value;

            simdjson::simdjson_result Result = RawArray.at(1);
            ondemand::value ValueRaw;

            TachyonAssert(Result.error() == error_code::SUCCESS);
            TachyonAssert(Result.get(ValueRaw) == error_code::SUCCESS);

            Value.Value = SanitizeData(ValueRaw);
            Value.PrimitiveType = PrimitiveType;

            RawArray.reset();

            return Value;
        }
    }
    __unreachable;
}

ScratchMutation ScratchBlock::ParseMutation(ondemand::object MutationObject) {
    ScratchMutation BlockMutation;
    if (this->Opcode == "procedures_prototype" || this->Opcode == "procedures_call") {
        /*
            proccode
        */
        simdjson::simdjson_result Result = MutationObject.find_field_unordered("proccode");
        TachyonAssert(Result.error() == error_code::SUCCESS);
        TachyonAssert(Result.get_string().get(BlockMutation.ProcCode) == error_code::SUCCESS);
        /*
            warp
        */
        Result = MutationObject.find_field_unordered("warp");
        TachyonAssert(Result.error() == error_code::SUCCESS);
        std::string UseWarpString;
        TachyonAssert(Result.get_string().get(UseWarpString) == error_code::SUCCESS);
        BlockMutation.UseWarp = (UseWarpString == "true");
        /*
            argumentids
        */
        Result = MutationObject.find_field_unordered("argumentids");
        TachyonAssert(Result.error() == error_code::SUCCESS);
        std::string ParamKeysString;
        TachyonAssert(Result.get_string().get(ParamKeysString) == error_code::SUCCESS);
        padded_string ParamKeysJSON(ParamKeysString);

        ondemand::parser KeyParser;
        auto ParamKeys = KeyParser.iterate(ParamKeysJSON);
        for(auto KeyValue : ParamKeys.get_array()) {
            std::string Key;
            TachyonAssert(KeyValue.get_string().get(Key) == error_code::SUCCESS);

            BlockMutation.ParametersKeys.push_back(
                Key
            );
        }
        if (this->Opcode == "procedures_prototype") {
            /* has extras */
            ondemand::parser NameParser;
            ondemand::parser DefaultsParser;
            /*
                argumentnames
            */
            Result = MutationObject.find_field_unordered("argumentnames");
            TachyonAssert(Result.error() == error_code::SUCCESS);
            std::string ParamNamesString;
            TachyonAssert(Result.get_string().get(ParamNamesString) == error_code::SUCCESS);
            padded_string ParamNamesJSON(ParamNamesString);
            auto ParamNames = NameParser.iterate(ParamNamesJSON);
            for(auto NameValue : ParamNames.get_array()) {
                std::string Name;
                TachyonAssert(NameValue.get_string().get(Name) == error_code::SUCCESS);

                BlockMutation.ParametersNames.push_back(
                    Name
                );
            }
            /*
                argumentdefaults
            */
            Result = MutationObject.find_field_unordered("argumentdefaults");
            TachyonAssert(Result.error() == error_code::SUCCESS);
            std::string ParamDefaultsString;
            TachyonAssert(Result.get_string().get(ParamDefaultsString) == error_code::SUCCESS);
            padded_string ParamDefaultsJSON(ParamDefaultsString);
            auto ParamDefaults = DefaultsParser.iterate(ParamDefaultsJSON);
            for(auto DefaultValueField : ParamDefaults.get_array()) {
                ondemand::value DefaultValue;
                TachyonAssert(DefaultValueField.get(DefaultValue) == error_code::SUCCESS);
                BlockMutation.ParameterDefaults.push_back(
                    SanitizeData(DefaultValue)
                );
            }
        }
    } else {
        simdjson::simdjson_result Result = MutationObject.find_field_unordered("hasnext");
        TachyonAssert(Result.error() == error_code::SUCCESS);
        std::string HasNextString;
        TachyonAssert(Result.get_string().get(HasNextString) == error_code::SUCCESS);
        BlockMutation.HasNext = (HasNextString == "true");
    }
    return BlockMutation;
}

static inline ScratchInput::InputType GetInputType(std::string & Key) {
    /* these all belong to the same type (ScratchInput::InputType::ValueInput) */
    static const std::unordered_set<std::string> ValueKeys = {
        "VALUE", "MESSAGE", "STRING1", "STRING2",
        "OPERAND1", "OPERAND2", "ITEM", "INDEX",
        "TIMES", "NUM1", "NUM2", "STRING",
        "NUM", "LETTER", "OPERAND", "DURATION",
        "COSTUME", "SIZE", "X", "Y",
        "DIRECTION", "STEPS", "DEGREES", "TO",
        "VOLUME", "SOUND_MENU", "BACKDROP", "CLONE_OPTION",
        "FROM", "TO", "DX", "DY",
        "OBJECT", "CHANGE", "SECS", "KEY_OPTION",
        "TOUCHINGOBJECTMENU", "TOWARDS", "DISTANCETOMENU", 
    };
    if (ValueKeys.count(Key) > 0) return ScratchInput::InputType::ValueInput;
    if (Key == "CONDITION") return ScratchInput::InputType::ConditionInput;
    if (Key == "SUBSTACK" || Key == "SUBSTACK2") return ScratchInput::InputType::SubstackInput;
    if (Key == "BROADCAST_INPUT") return ScratchInput::InputType::BroadcastInput;
    if (Key == "custom_block") return ScratchInput::InputType::ProcedureDefinition;
    /* bad input */
    return ScratchInput::InputType::InvalidInput;
}

static inline ScratchField::FieldType GetFieldType(std::string & Key) {
    /* these all belong to the same type (ScratchField::FieldType::StringField) */
    static const std::unordered_set<std::string> ValueKeys = {
        "OPERATOR", "VALUE", "STOP_OPTION", "CURRENTMENU",
        "COSTUME", "TO", "SOUND_MENU", "BACKDROP",
        "CLONE_OPTION", "EFFECT", "PROPERTY", "OBJECT",
        "FRONT_BACK", "KEY_OPTION", "TOUCHINGOBJECTMENU", "TOWARDS",
        "NUMBER_NAME", "DISTANCETOMENU", 
    };
    if (ValueKeys.count(Key) > 0) return ScratchField::FieldType::StringField;
    if (Key == "LIST") return ScratchField::FieldType::ListField;
    if (Key == "VARIABLE") return ScratchField::FieldType::VariableField;
    if (Key == "BROADCAST_OPTION") return ScratchField::FieldType::BroadcastOption;
    /* bad input */
    return ScratchField::FieldType::InvalidField;
}

static inline void ParseValueInput(ScratchInput & Input, ondemand::array & InputObject) {
    Input_Value Value;
    switch(Input.ShadowType) {
        case ScratchShadow::INPUT_SAME_BLOCK_SHADOW: {
            InputObject.reset();

            simdjson::simdjson_result Result = InputObject.at(1);
            TachyonAssert(Result.error() == error_code::SUCCESS);

            bool IsString;
            TachyonAssert(Result.is_string().get(IsString) == error_code::SUCCESS);

            ondemand::array ValueArray;

            if (IsString) {
                std::string ReporterKeyString;

                TachyonAssert(Result.get_string().get(ReporterKeyString) == error_code::SUCCESS);

                InputObject.reset();

                Input.ReporterKey = ReporterKeyString;
                Input.Input = ReporterKeyString;
                Input.HasReporter = true;

                InputObject.reset();
                break;
            }

            TachyonAssert(Result.get_array().get(ValueArray) == error_code::SUCCESS);

            InputObject.reset();
            /* these usually dont have reporters */
            Input.Input = SetupInputValue(ValueArray);
            Input.HasReporter = false;

            InputObject.reset();
            break;
        }
        case ScratchShadow::INPUT_BLOCK_NO_SHADOW: {
            simdjson::simdjson_result Result = InputObject.at(1);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            
            Input.HasReporter = true;

            std::string ReporterId;

            TachyonAssert(Result.get_string().get(ReporterId) == error_code::SUCCESS);
            
            Input.Input = ReporterId;
            Input.ReporterKey = ReporterId;
            break;
        }
        case ScratchShadow::INPUT_DIFF_BLOCK_SHADOW: {
            ondemand::array ValueArray;

            InputObject.reset();

            simdjson::simdjson_result Result = InputObject.at(1);
            TachyonAssert(Result.error() == error_code::SUCCESS);

            bool IsString;
            TachyonAssert(Result.is_string().get(IsString) == error_code::SUCCESS);

            if (IsString) {
                /* the typical block chain */
                std::string ReporterKeyString;

                TachyonAssert(Result.get_string().get(ReporterKeyString) == error_code::SUCCESS);

                InputObject.reset();

                Result = InputObject.at(2);
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.is_string().get(IsString) == error_code::SUCCESS);

                if (IsString == false) {
                    TachyonAssert(Result.get_array().get(ValueArray) == error_code::SUCCESS);
                    InputObject.reset();

                    /* these always have a reporter */
                    Input.ReporterKey = ReporterKeyString;
                    Input.Input = SetupInputValue(ValueArray);
                    Input.HasReporter = true;

                    InputObject.reset();
                    break;
                }
                /* can be safely ignored */
                break;
            }
            /* variable, list, or broadcast */
            TachyonAssert(Result.get_array().get(ValueArray) == error_code::SUCCESS);
            Input.Input = SetupInputValue(ValueArray);
            Input.HasReporter = false;

            InputObject.reset();
            break;
        }
    }
}

/**
 * Control input = substack / condition
 * @param Input
 * @param JSON data of the input
 */
static inline void ParseControlInput(ScratchInput & Input, ondemand::array & InputObject) {
    InputObject.reset();

    simdjson::simdjson_result Result = InputObject.at(1);
    TachyonAssert(Result.error() == error_code::SUCCESS);

    bool IsNull;
    TachyonAssert(Result.is_null().get(IsNull) == error_code::SUCCESS);

    Input.HasReporter = false;
    if (IsNull == true) {
        /* empty */
        Input.Input = {};
        return;
    }

    std::string String;
    TachyonAssert(Result.get_string().get(String) == error_code::SUCCESS);
    Input.Input = String;

    InputObject.reset();
}

static inline void ParseProcedureDefinition(ScratchInput & Input, ondemand::array & InputObject) {
    InputObject.reset();

    simdjson::simdjson_result Result = InputObject.at(1);

    TachyonAssert(Result.error() == error_code::SUCCESS);
    std::string ProcDefString;
    TachyonAssert(Result.get_string().get(ProcDefString) == error_code::SUCCESS);

    Input.Input = ProcDefString;
    Input.HasReporter = false;

    InputObject.reset();
}

ScratchInput ScratchBlock::ParseInput(std::string & Key, ondemand::array InputObject) {
    ScratchInput Input;
    Input.ShadowType = ParseShadowType(InputObject);
    Input.Type = GetInputType(Key);

    switch(Input.Type) {
        case ScratchInput::InputType::ValueInput: {
            ParseValueInput(Input, InputObject);
            break;
        }
        case ScratchInput::InputType::ConditionInput:
        case ScratchInput::InputType::SubstackInput: {
            ParseControlInput(Input, InputObject);
            break;
        }
        case ScratchInput::InputType::BroadcastInput: {
            ParseValueInput(Input, InputObject);
            break;
        }
        case ScratchInput::InputType::ProcedureDefinition: {
            ParseProcedureDefinition(Input, InputObject);
            break;
        }
        case ScratchInput::InputType::InvalidInput: {
            if (this->IsProcedurePrototype() == false && this->IsProcedureCall() == false) {
                TachyonUnimplemented("Unknown input. Input: %u, Key: %s\n", Input.Type, Key.c_str());
            }
            Input.Type = ScratchInput::InputType::ValueInput;
            ParseValueInput(Input, InputObject);
            break;
        }
    }
    return Input;
}

static inline void ParseBroadcastField(ScratchField & Field, ondemand::array & FieldObject) {
    FieldObject.reset();

    simdjson::simdjson_result Result = FieldObject.at(1);
    TachyonAssert(Result.error() == error_code::SUCCESS);

    std::string BroadcastOption;
    TachyonAssert(Result.get_string().get(BroadcastOption) == error_code::SUCCESS);

    Field.Field = BroadcastOption;

    FieldObject.reset();
}

static inline void ParseStringOption(ScratchField & Field, ondemand::array & FieldObject) {
    FieldObject.reset();

    simdjson::simdjson_result Result = FieldObject.at(0);
    TachyonAssert(Result.error() == error_code::SUCCESS);

    std::string StringOption;
    TachyonAssert(Result.get_string().get(StringOption) == error_code::SUCCESS);

    Field.Field = StringOption;

    FieldObject.reset();
}

static inline void ParseDataField(ScratchField & Field, ondemand::array & FieldObject, ScratchBlock & Block) {
    simdjson::simdjson_result Result = FieldObject.at(0);
    TachyonAssert(Result.error() == error_code::SUCCESS);

    std::string VariableName;
    TachyonAssert(Result.get_string().get(VariableName) == error_code::SUCCESS);
    FieldObject.reset();

    Result = FieldObject.at(1);
    std::string VariableKey;
    TachyonAssert(Result.get_string().get(VariableKey) == error_code::SUCCESS);
    FieldObject.reset();

    ScratchSprite & Owner = Block.GetOwnerSprite();
    Field_Variable VariableField;

    VariableField.VariableKey = VariableKey;
    VariableField.VariableName = VariableName;
    Field.Field = VariableField;
}

ScratchField ScratchBlock::ParseField(std::string & Key, ondemand::array FieldObject) {
    ScratchField Field;
    Field.Type = GetFieldType(Key);
    switch(Field.Type) {
        case ScratchField::FieldType::ListField:
        case ScratchField::FieldType::VariableField: {
            ParseDataField(Field, FieldObject, *this);
            break;
        }
        case ScratchField::FieldType::StringField: {
            ParseStringOption(Field, FieldObject);
            break;
        }
        case ScratchField::FieldType::BroadcastOption: {
            ParseBroadcastField(Field, FieldObject);
            break;
        }
        case ScratchField::FieldType::InvalidField: {
            TachyonUnimplemented("Unknown field. Field: %u, Key: %s\n", Field.Type, Key.c_str());
        }
    }
    return Field;
}