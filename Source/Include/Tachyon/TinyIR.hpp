#pragma once

#include <unordered_map>
#include <optional>
#include <vector>
#include <bit>

#include <Scratch/Blocks.hpp>
#include <Scratch/Procedures.hpp>
#include <Lib/NanBox.hpp>

using namespace Scratch;

namespace TinyIR {
    class IRValue {
        private:
            std::vector<NanBox::BoxedValue> Values = {};
        public:
            /* liveness range */
            size_t FirstUsed;
            size_t LastUsed;
            std::optional<size_t> ParameterNum = std::nullopt;
            /* register allocation */
            size_t AllocatorId;
            bool StackSpilled;
            bool TempPushed;

            /**
             * no value
             */
            explicit constexpr IRValue() {
                this->Values.reserve(16);
            }

            explicit constexpr IRValue(const NanBox::BoxedValue Value) {
                this->Values.reserve(16);
                this->Write(Value);
            }

            inline void Write(const NanBox::BoxedValue NewValue) {
                this->Values.insert(this->Values.begin() + this->Values.size(), NewValue);
            }

            constexpr size_t GetValueCursor(void) const {
                return this->Values.size();
            }

            constexpr NanBox::BoxedValue GetValue(void) const {
                return this->Values.back();
            }

            constexpr auto & GetValues(void) {
                return this->Values;
            }
    };

    enum IROpcodeType : uint8_t {
        /* misc. */
        NOP,    // no operation
        LPARAM, // link parameter
        /* data ops */
        SET,    // set value
        LOAD,   // load value
        /* basic arithmetic ops */
        ADD,    // add
        SUB,    // subtract
        MUL,    // multiply
        DIV,    // divide
        /* logic ops */
        CMP,    // compare
        JNE,    // jump if NOT equal
        JE,     // jump if equal
        JG,     // jump if greater than
        JL,     // jump if less than
        JGE,    // jump if greater or equal
        JLE,    // jump if less than or equal
        /* flow control */
        JMP,    // jump to block
        CALL,   // call procedure
        RCALL,  // call runtime procedure
        RCALLV, // call runtime procedure (with return value)
        RET,    // return from procedure
        /* runtime one-time ops */
        START,  // emit runtime prologue
        END,    // emit runtime epilogue
        /* function one-time ops */
        ENTER,  // emit function prologue
        LEAVE,  // emit function epilogue
    };

    enum IRRCallType : uint8_t {
        INVALID,
        LOOKS_SAY,
    };

    class IROpcode {
        private:
            std::vector<size_t> Inputs;
            const IROpcodeType Type;
            const std::optional<IRRCallType> RCallType  = std::nullopt; 
        public:
            ScratchBlock * const Block = nullptr;
            ScratchProcedure * const Procedure = nullptr;
            std::array<ScratchBlock *, 2> BlockLinks = { nullptr, nullptr };
            explicit constexpr IROpcode(ScratchBlock * const SomeBlock, IROpcodeType Type) : Block(SomeBlock), Type(Type) {}
            explicit constexpr IROpcode(ScratchBlock * const SomeBlock, IROpcodeType Type, ScratchProcedure * const ConnectedProcedure) : Block(SomeBlock), Type(Type), Procedure(ConnectedProcedure) {}
            explicit constexpr IROpcode(ScratchBlock * const SomeBlock, IROpcodeType Type, IRRCallType RCallType) : Block(SomeBlock), Type(Type), RCallType(RCallType) {}

            constexpr IRRCallType GetRCallType(void) const {
                return this->RCallType.value_or(IRRCallType::INVALID);
            }

            constexpr IROpcodeType GetType(void) const {
                return this->Type;
            }

            constexpr size_t GetNumInputs(void) const {
                return this->Inputs.size();
            }

            constexpr const auto & GetInputs(void) const {
                return this->Inputs;
            }

            constexpr void PushValue(const size_t AssignmentNum) {
                this->Inputs.insert(this->Inputs.begin() + this->Inputs.size(), AssignmentNum);
            }
    };

    using IRStack = std::vector<TinyIR::IROpcode>;
    using IRValueLocator = std::pair<size_t, IRValue *>;

    class IRGenerator {
        private:
            IRStack BlockStack;

            std::unordered_map<void *, IRValueLocator> DataBinds; // 1st = data pointer, 2nd = ir value pointer
            std::unordered_map<std::string, size_t> ParameterBinds;
            std::vector<IRValue> Variables;

            size_t BlockCounter;

            size_t TotalParams = 0;
            bool ProcedureContext = false;

            void DescendInputs(IROpcode & Opcode);

            inline size_t AssignValue(const NanBox::BoxedValue Boxed) {
                IRValue Value(Boxed);

                Value.FirstUsed = this->BlockCounter;
                Value.LastUsed = Value.FirstUsed;

                DebugInfo("v%d = 0x%016zx\n", this->Variables.size(), Boxed);
                this->Variables.insert(this->Variables.begin() + this->Variables.size(), Value);
                return this->Variables.size() - 1;
            }

            inline size_t AssignReference(const size_t Reference) {
                IRValue & Value = this->GetVariable(Reference);

                Value.LastUsed = this->BlockCounter;

                DebugInfo("v%d = v%d\n", this->Variables.size(), Reference);
                this->Variables.insert(this->Variables.begin() + this->Variables.size(), Value);
                return this->Variables.size() - 1;
            }

            inline size_t AssignParameter(void) {
                IRValue Value;
                Value.ParameterNum = this->TotalParams;
                Value.LastUsed = this->BlockCounter;

                DebugInfo("register p%d (pseudo-opcode)\n", this->TotalParams++);

                this->Variables.insert(this->Variables.begin() + this->Variables.size(), Value);
                return this->Variables.size() - 1;
            }

            void BindProcedureParameters(ScratchBlock & ProcedureDefinition);

            IROpcode GenerateOpcode(ScratchBlock * const Block);
        public:
            template <typename T>
            inline void BindData(T * DataPtr, size_t VariableNum) {
                IRValue & Value = this->GetVariable(VariableNum);
                DebugInfo("bind %c%d, 0x%p (pseudo-opcode)\n", Value.ParameterNum.has_value() ? 'p' : 'v', VariableNum, DataPtr);
                this->DataBinds.emplace(std::bit_cast<void *>(DataPtr), std::make_pair(VariableNum, &Value));
            }

            constexpr auto & GetDataBinds(void) {
                return this->DataBinds;
            }

            [[nodiscard]]
            constexpr IRValue & GetVariable(size_t VariableNum) {
                TachyonAssert(VariableNum <= this->Variables.size());
                return this->Variables[VariableNum];
            }

            constexpr NanBox::BoxedValue GetVariableData(size_t VariableNum) {
                TachyonAssert(VariableNum <= this->Variables.size());
                return this->Variables[VariableNum].GetValue();
            }

            constexpr auto & GetVariables(void) {
                return this->Variables;
            }

            constexpr const auto & GetVariables(void) const {
                return this->Variables;
            }

            constexpr bool IsProcedureContext(void) const {
                return this->ProcedureContext;
            }
            
            [[nodiscard]]
            const IRStack & GenerateIR(ScratchBlock & Hat);
    };
};