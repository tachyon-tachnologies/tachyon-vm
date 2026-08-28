#include <unordered_set>

#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Lib/NanBox.hpp>
#include <simdjson.h>
#include <Common.hpp>

using namespace simdjson;
using namespace Scratch;

BoxedValue __hot ScratchBlock::GetInputData(size_t InputNum) {
    /* bad InputNum */
    if (unlikely(InputNum >= this->Inputs.size())) {
        return Box(0.0);
    }
    ScratchInput & Input = this->Inputs[InputNum];
    if (Input.IsReporter() == true) {
        /* get it from the connected block instead */
        ScratchSprite & OwnerSprite = this->Sprite.get();
        ScratchBlock * Reporter = Input.ReporterBlock;

        TachyonAssert(Reporter != nullptr);

        return Reporter->Evaluate();
    }
    /* a normal input value */
    if (Input.IsType(InputType::ValueInput)) {
        Input_Value InputValue = std::get<Input_Value>(Input.Input);
        ScratchSprite & OwnerSprite = this->GetOwnerSprite();
        if (auto VariablePtr = std::get_if<ScratchVariable *>(&InputValue.Value)) {
            ScratchVariable * Variable = *VariablePtr;
            return Variable->GetData();
        } else if (auto ListPtr = std::get_if<ScratchList *>(&InputValue.Value)) {
            /* UNIMPLEMENTED */
            return Box(0.0);
        } else if (auto ValuePtr = std::get_if<BoxedValue>(&InputValue.Value)) {
            return *ValuePtr;
        }
    }
    return Box(0.0);
}

ScratchInput & __hot ScratchBlock::GetInput(size_t InputNum) {
    TachyonAssertMsg(!((InputNum > this->Inputs.size() - 1) || this->Inputs.empty()), "Invalid input number %d!\n", InputNum);
    return this->Inputs[InputNum];
}

ScratchField & __hot ScratchBlock::GetField(size_t FieldNum) {
    TachyonAssertMsg(!((FieldNum > this->Fields.size() - 1) || this->Fields.empty()), "Invalid field number %d!\n", FieldNum);
    return this->Fields[FieldNum];
}

ScratchSprite & __hot ScratchBlock::GetOwnerSprite(void) const {
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

static std::variant<Input_Value, std::string> SetupInputValue(ScratchSprite & Owner, ondemand::array & RawArray) {
    ScratchPrimitive PrimitiveType = ParsePrimitiveType(RawArray);

    switch(PrimitiveType) {
        case ScratchPrimitive::INPUT_VAR: {
            Input_Value Value;

            std::string VariableKey;

            simdjson::simdjson_result Result = RawArray.at(2);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            TachyonAssert(Result.get_string().get(VariableKey) == error_code::SUCCESS);
            
            RawArray.reset();

            Value.PrimitiveType = PrimitiveType;
            Value.Value = Owner.GetVariableFromKey(VariableKey);

            RawArray.reset();

            return Value;
        }
        case ScratchPrimitive::INPUT_LIST: {
            Input_Value Value;

            std::string ListKey;

            simdjson::simdjson_result Result = RawArray.at(2);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            TachyonAssert(Result.get_string().get(ListKey) == error_code::SUCCESS);

            RawArray.reset();

            Value.PrimitiveType = PrimitiveType;
            Value.Value = Owner.GetListFromKey(ListKey);

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

static inline InputType GetInputType(std::string_view Key) {
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
    if (ValueKeys.count(Key.data()) > 0) return InputType::ValueInput;
    if (Key == "CONDITION") return InputType::ConditionInput;
    if (Key == "SUBSTACK" || Key == "SUBSTACK2") return InputType::SubstackInput;
    if (Key == "BROADCAST_INPUT") return InputType::BroadcastInput;
    if (Key == "custom_block") return InputType::ProcedureDefinition;
    /* bad input */
    return InputType::InvalidInput;
}

static inline FieldType GetFieldType(std::string_view Key) {
    /* these all belong to the same type (ScratchField::FieldType::StringField) */
    static const std::unordered_set<std::string> ValueKeys = {
        "OPERATOR", "VALUE", "STOP_OPTION", "CURRENTMENU",
        "COSTUME", "TO", "SOUND_MENU", "BACKDROP",
        "CLONE_OPTION", "EFFECT", "PROPERTY", "OBJECT",
        "FRONT_BACK", "KEY_OPTION", "TOUCHINGOBJECTMENU", "TOWARDS",
        "NUMBER_NAME", "DISTANCETOMENU",
    };
    if (ValueKeys.count(Key.data()) > 0) return FieldType::StringField;
    if (Key == "LIST") return FieldType::ListField;
    if (Key == "VARIABLE") return FieldType::VariableField;
    if (Key == "BROADCAST_OPTION") return FieldType::BroadcastOption;
    /* bad input */
    return FieldType::InvalidField;
}

void ScratchInput::ParseValueInput(ScratchSprite & Owner, ondemand::array & InputObject) {
    Input_Value Value;
    switch(this->ShadowType) {
        case ScratchShadow::INPUT_SAME_BLOCK_SHADOW: {
            InputObject.reset();

            auto Result = InputObject.at(1);
            TachyonAssert(Result.error() == error_code::SUCCESS);

            bool IsString;
            TachyonAssert(Result.is_string().get(IsString) == error_code::SUCCESS);

            ondemand::array ValueArray;

            if (IsString) {
                std::string ReporterKeyString;

                TachyonAssert(Result.get_string().get(ReporterKeyString) == error_code::SUCCESS);

                InputObject.reset();

                // this->ReporterBlock = Owner.GetBlockFromId(ReporterKeyString);
                this->Input = ReporterKeyString;
                this->Reporter = true;

                InputObject.reset();
                break;
            }

            TachyonAssert(Result.get_array().get(ValueArray) == error_code::SUCCESS);

            InputObject.reset();
            /* these usually dont have reporters */
            this->Input = SetupInputValue(Owner, ValueArray);
            this->Reporter = false;

            InputObject.reset();
            break;
        }
        case ScratchShadow::INPUT_BLOCK_NO_SHADOW: {
            simdjson::simdjson_result Result = InputObject.at(1);
            TachyonAssert(Result.error() == error_code::SUCCESS);
            
            this->Reporter = true;

            std::string ReporterId;

            TachyonAssert(Result.get_string().get(ReporterId) == error_code::SUCCESS);
            
            this->Input = ReporterId;
            // this->ReporterBlock = Owner.GetBlockFromId(ReporterId);
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

                this->Input = ReporterKeyString;
                this->Reporter = true;
                break;
            }
            /* variable, list, or broadcast */
            TachyonAssert(Result.get_array().get(ValueArray) == error_code::SUCCESS);
            this->Input = SetupInputValue(Owner, ValueArray);
            this->Reporter = false;

            InputObject.reset();
            break;
        }
    }
}

void ScratchInput::ParseControlInput(ondemand::array & InputObject) {
    InputObject.reset();

    simdjson::simdjson_result Result = InputObject.at(1);
    TachyonAssert(Result.error() == error_code::SUCCESS);

    bool IsNull;
    TachyonAssert(Result.is_null().get(IsNull) == error_code::SUCCESS);

    this->Reporter = false;
    if (IsNull == true) {
        /* empty */
        this->Input = {};
        return;
    }

    std::string String;
    TachyonAssert(Result.get_string().get(String) == error_code::SUCCESS);
    this->Input = String;

    InputObject.reset();
}

void ScratchInput::ParseProcedureDefinition(ondemand::array & InputObject) {
    InputObject.reset();

    simdjson::simdjson_result Result = InputObject.at(1);

    TachyonAssert(Result.error() == error_code::SUCCESS);
    std::string ProcDefString;
    TachyonAssert(Result.get_string().get(ProcDefString) == error_code::SUCCESS);

    this->Input = ProcDefString;
    this->Reporter = false;

    InputObject.reset();
}

ScratchInput::ScratchInput(ScratchBlock & Owner, const std::string & Key, ondemand::array & InputObject) {
    this->ShadowType = ParseShadowType(InputObject);
    this->Type = GetInputType(Key);

    ScratchSprite & OwnerSprite = Owner.GetOwnerSprite();

    switch(this->Type) {
        case InputType::ValueInput: {
            this->ParseValueInput(OwnerSprite, InputObject);
            break;
        }
        case InputType::ConditionInput:
        case InputType::SubstackInput: {
            this->ParseControlInput(InputObject);
            break;
        }
        case InputType::BroadcastInput: {
            this->ParseValueInput(Owner.GetOwnerSprite(), InputObject);
            break;
        }
        case InputType::ProcedureDefinition: {
            this->ParseProcedureDefinition(InputObject);
            break;
        }
        default: {
            if (Owner.IsProcedurePrototype() == false && Owner.IsProcedureCall() == false) {
                TachyonUnimplemented("Unknown input. Input: %u, Key: %s\n", this->Type, Key.data());
            }
            this->Type = InputType::ValueInput;
            this->ParseValueInput(OwnerSprite, InputObject);
            break;
        }
    }
}

void ScratchField::ParseBroadcastField(ondemand::array & FieldObject) {
    FieldObject.reset();

    simdjson::simdjson_result Result = FieldObject.at(1);
    TachyonAssert(Result.error() == error_code::SUCCESS);

    std::string BroadcastOption;
    TachyonAssert(Result.get_string().get(BroadcastOption) == error_code::SUCCESS);

    this->Value = BroadcastOption;

    FieldObject.reset();
}

void ScratchField::ParseStringOption(ondemand::array & FieldObject) {
    FieldObject.reset();

    simdjson::simdjson_result Result = FieldObject.at(0);
    TachyonAssert(Result.error() == error_code::SUCCESS);

    std::string StringOption;
    TachyonAssert(Result.get_string().get(StringOption) == error_code::SUCCESS);

    this->Value = StringOption;

    FieldObject.reset();
}

void ScratchField::ParseDataField(ondemand::array & FieldObject, ScratchBlock & Block) {
    simdjson::simdjson_result Result = FieldObject.at(1);
    TachyonAssert(Result.error() == error_code::SUCCESS);

    std::string VariableKey;
    TachyonAssert(Result.get_string().get(VariableKey) == error_code::SUCCESS);
    FieldObject.reset();

    ScratchSprite & Owner = Block.GetOwnerSprite();
    if (ScratchList * List = Owner.GetListFromKey(VariableKey)) {
        this->Value = List;
        return;
    } else if (ScratchVariable * Variable = Owner.GetVariableFromKey(VariableKey)) {
        this->Value = Variable;
        return;
    }
    TachyonAbort("Invalid data key: \"%s\"\n", VariableKey.data());
}

ScratchField::ScratchField(ScratchBlock & Owner, const std::string & Key, ondemand::array & FieldObject) {
    this->Type = GetFieldType(Key);
    switch(this->Type) {
        case FieldType::VariableField:
        case FieldType::ListField: {
            this->ParseDataField(FieldObject, Owner);
            break;
        }
        case FieldType::StringField: {
            this->ParseStringOption(FieldObject);
            break;
        }
        case FieldType::BroadcastOption: {
            this->ParseBroadcastField(FieldObject);
            break;
        }
        default: {
            TachyonUnimplemented("Unknown field. Field: %u, Key: %s\n", this->Type, Key.data());
        }
    }
}
