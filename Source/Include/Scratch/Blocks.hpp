#pragma once

#include <string>

#include <Tachyon/Debug.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Status.hpp>
#include <Lib/NanBox.hpp>
#include <Common.hpp>
#include <simdjson.h>

using namespace NanBox;
using namespace simdjson;

namespace Scratch {
    class ScratchSprite;

    class ScratchBlock;

    /**
     * Contains block mutation information.
     */
    struct ScratchMutation {
        std::vector<std::string> ParametersKeys;
        std::vector<std::string> ParametersNames;
        std::vector<BoxedValue> ParameterDefaults;
        std::string ProcCode;
        bool HasNext;
        bool UseWarp;
    };
    

    /**
     * Scratch opcode function handler.
     */

    using OpcodeHandler = ScratchStatus (*)(ScratchBlock &);
    using EvaluationHandler = BoxedValue (*)(ScratchBlock &);

    /**
     * Contains scratch block information.
     */
    class ScratchBlock {
        public:
            /**
             * Pointer to the next block
             */
            ScratchBlock * NextBlock_Pointer = nullptr;

            /**
             * Scratch block constructor.
             * @param Key The block's key ID.
             * @param BlockData The block's JSON data.
             * @param Owner The owner of the block
             */
            ScratchBlock (std::string_view Key, ondemand::object & BlockData, ScratchSprite & Owner) : Sprite(Owner), BlockKey(Key) {
                BlockData.reset();
                /*
                    opcode
                */
                auto Result = BlockData.find_field_unordered("opcode");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Opcode) == error_code::SUCCESS);
                /*
                    topLevel
                */
                Result = BlockData.find_field_unordered("topLevel");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_bool().get(this->TopLevel) == error_code::SUCCESS);
                /*
                    shadow
                */
                Result = BlockData.find_field_unordered("shadow");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_bool().get(this->Shadow) == error_code::SUCCESS);

                this->ProcedureDefinition = (this->Opcode == "procedures_definition");
                this->ProcedurePrototype = (this->Opcode == "procedures_prototype");
                this->ProcedureCall = (this->Opcode == "procedures_call");
                this->GreenFlag = (this->Opcode == "event_whenflagclicked");

                /*
                    next
                */
                Result = BlockData.find_field_unordered("next");
                TachyonAssertMsg(Result.error() == error_code::SUCCESS, "No next block field??");
                bool IsNull;
                TachyonAssert(Result.is_null().get(IsNull) == error_code::SUCCESS);
                if (IsNull == false) {
                    TachyonAssert(Result.get_string().get(this->NextBlock_Key) == error_code::SUCCESS);
                }
                /*
                    parent
                */
                Result = BlockData.find_field_unordered("parent");
                TachyonAssertMsg(Result.error() == error_code::SUCCESS, "No parent block field??");
                TachyonAssert(Result.is_null().get(IsNull) == error_code::SUCCESS);
                if (IsNull == false) {
                    TachyonAssert(Result.get_string().get(this->ParentBlock_Key) == error_code::SUCCESS);
                }
                /*
                    mutation
                */
                Result = BlockData.find_field_unordered("mutation");
                if (Result.error() == error_code::SUCCESS) {
                    ondemand::object MutationObject;
                    TachyonAssert(Result.get_object().get(MutationObject) == error_code::SUCCESS);
                    Mutation = this->ParseMutation(MutationObject);
                }
                /* 
                    inputs 
                */
                Result = BlockData.find_field_unordered("inputs");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::object InputData;
                TachyonAssert(Result.get_object().get(InputData) == error_code::SUCCESS);
                for (auto InputField : InputData) {
                    std::string InputKey;
                    TachyonAssert(InputField.escaped_key().get(InputKey) == error_code::SUCCESS);

                    ondemand::array InputArray;
                    TachyonAssert(InputField.value().get_array().get(InputArray) == error_code::SUCCESS);

                    this->Inputs.emplace_back(
                        ScratchInput(*this, InputKey, InputArray)
                    );
                }
                if (this->Inputs.empty() == false) {
                    /* sort inputs */
                    std::sort(this->Inputs.begin(), this->Inputs.end(), [](const ScratchInput & A, const ScratchInput & B) {
                        return A.GetType() < B.GetType();
                    });
                }
                /*
                    fields
                */
                Result = BlockData.find_field_unordered("fields");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::object FieldData;
                TachyonAssert(Result.get_object().get(FieldData) == error_code::SUCCESS);
                for (auto FieldField : FieldData) {
                    std::string FieldKey;
                    TachyonAssert(FieldField.escaped_key().get(FieldKey) == error_code::SUCCESS);

                    ondemand::array FieldArray;
                    TachyonAssert(FieldField.value().get_array().get(FieldArray) == error_code::SUCCESS);

                    // WARNING: watch out for lifetime
                    this->Fields.emplace_back(
                        ScratchField(*this, FieldKey, FieldArray)
                    );
                }
                /* assign function based on opcode */
                this->LinkHandlers();
            }

            /**
             * Gets the block's opcode.
             * @return The block's opcode.
             */
            constexpr const std::string & GetOpcode(void) const {
                return this->Opcode;
            }

            /**
             * Gets the next block's key.
             * @return The next block's key.
             */
            constexpr const std::string & __hot GetNextKey(void) const {
                return this->NextBlock_Key;
            }

            /**
             * Gets the parent block's key.
             * @return The parent block's key.
             */
            constexpr const std::string & __hot GetParentKey(void) const {
                return this->ParentBlock_Key;
            }

            /**
             * Gets the parent block's key.
             * @return The parent block's key.
             */
            constexpr const std::string & __hot GetKey(void) const {
                return this->BlockKey;
            }
            
            /**
             * Checks if the block is a green flag block.
             * @return True if it is a green flag block, false if otherwise.
             */
            constexpr bool IsGreenFlag(void) const {
                return this->GreenFlag;
            }

            /**
             * Checks if the block is a procedure definition.
             * @return True if it's a procedure definition, false if otherwise.
             */
            constexpr bool IsProcedureDef(void) const {
                return this->ProcedureDefinition;
            }

            /**
             * Checks if the block is a procedure definition.
             * @return True if it's a procedure definition, false if otherwise.
             */
            constexpr bool IsProcedurePrototype(void) const {
                return this->ProcedurePrototype;
            }

            /**
             * Checks if the block is a procedure definition.
             * @return True if it's a procedure definition, false if otherwise.
             */
            constexpr bool IsProcedureCall(void) const {
                return this->ProcedureCall;
            }

            /**
             * Checks if the block is an argument reporter.
             * @return True if it's an argument reporter, false if otherwise.
             */
            constexpr bool IsArgumentReporter(void) const {
                return this->ArgumentReporter;
            }
            
            /**
             * Executes the current block.
             * Should only be used for the interpreter.
             */
            inline ScratchStatus __hot Execute(void) {
                if (likely(this->Handler)) {
                    return this->Handler(*this);
                }
                DebugError("Invalid opcode: %s\n", this->Opcode.c_str());
                return ScratchStatus::SCRATCH_END;
            }

            /**
             * Executes and returns the block's result.
             * Should only be used for the interpreter.
             */
            inline BoxedValue __hot Evaluate(void) {
                if (likely(this->ReporterHandler)) {
                    return this->ReporterHandler(*this);
                }
                DebugWarn("Unknown reporter: %s\n", this->Opcode.c_str());
                return {};
            }

            /**
             * Gets the block's mutation (if it exists).
             */
            [[nodiscard]]
            constexpr ScratchMutation & __hot GetMutation(void) {
                return this->Mutation.value();
            }

            /**
             * Gets the total amount of inputs
             * @return Total inputs in the block
             */
            constexpr size_t GetNumInputs(void) const {
                return this->Inputs.size();
            }

            /**
             * Gets all the inputs in the block.
             * @returns A vector containing all the block's inputs
             */
            constexpr auto & GetAllInputs(void) {
                return this->Inputs;
            }

            /**
             * Gets all the fields in the block.
             * @returns A vector containing all the block's fields
             */
            constexpr auto & GetAllFields(void) {
                return this->Fields;
            }

            /**
             * Get's the block's sprite.
             * @return The block's sprite.
             */
            ScratchSprite & __hot GetOwnerSprite(void) const;
            
            BoxedValue __hot GetInputData(size_t InputNum);
            ScratchInput & __hot GetInput(size_t InputNum);
            ScratchField & __hot GetField(size_t FieldNum);
        private:
            std::optional<ScratchMutation> Mutation;

            /**
             * Links an opcode handler to the current block.
             */
            void LinkHandlers(void);

            ScratchMutation ParseMutation(ondemand::object MutationObject);
            ScratchInput ParseInput(std::string & Key, ondemand::array & InputObject);

            std::string Opcode;
            std::string NextBlock_Key;
            std::string ParentBlock_Key;
            std::string BlockKey;

            std::vector<ScratchInput> Inputs;
            std::vector<ScratchField> Fields;
            std::reference_wrapper<ScratchSprite> Sprite;

            /**
             * Should only be used for non-reporter blocks in interpreter mode.
             */
            OpcodeHandler Handler = nullptr;

            /**
             * Should only be used for reporter blocks in interpreter mode.
             */
            EvaluationHandler ReporterHandler = nullptr;

            bool Shadow;
            bool TopLevel;

            bool GreenFlag;
            bool ProcedureDefinition;
            bool ProcedurePrototype;
            bool ProcedureCall;
            bool ArgumentReporter;
    };
};
