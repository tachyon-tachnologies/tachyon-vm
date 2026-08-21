#pragma once

#include <Scratch/Blocks.hpp>
#include <Scratch/Procedures.hpp>
#include <vector>

using namespace Scratch;

namespace TinyIR {
    class IRValue {
        private:
            NanBox::BoxedValue Value;
        public:
            size_t FirstUse;
            size_t LastUse;

            explicit constexpr IRValue(NanBox::BoxedValue NotOurs) : Value(NotOurs) {}
            /* copy constructor */
            constexpr IRValue(const IRValue & Other) : FirstUse(Other.FirstUse), LastUse(Other.LastUse), Value(Other.Value) {}
            /* copy assignment constructor */
            constexpr IRValue & operator = (const IRValue & Other) {
                if (&Other == this) {
                    return *this;
                }
                this->FirstUse = Other.FirstUse;
                this->LastUse = Other.LastUse;
                this->Value = Other.Value;
                return *this;
            }
            /* move constructor */
            constexpr IRValue(IRValue && Other) : FirstUse(Other.FirstUse), LastUse(Other.LastUse), Value(Other.Value) {
                Other.FirstUse = 0;
                Other.LastUse = 0;
                Other.Value = 0;
            }
            /* move assignment constructor */
            constexpr IRValue & operator = (IRValue && Other) {
                if (&Other == this) {
                    return *this;
                }
                this->FirstUse = Other.FirstUse;
                this->LastUse = Other.LastUse;
                this->Value = Other.Value;

                Other.FirstUse = 0;
                Other.LastUse = 0;
                Other.Value = 0;

                return *this;
            }

            constexpr bool operator == (const NanBox::BoxedValue & Other) const {
                return (this->Value == Other);
            }

            constexpr bool operator != (const NanBox::BoxedValue & Other) const {
                return (this->Value != Other);
            }
    };

    enum IROpcodeType : uint8_t {
        /* misc. */
        NOP,    // no operation
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
        RET,    // return from procedure
        /* runtime one-time ops */
        START,  // emit runtime prologue
        END,    // emit runtime epilogue
        /* function one-time ops */
        ENTER,  // emit function prologue
        LEAVE,  // emit function epilogue
    };

    class IROpcode {
        private:
            std::vector<IRValue> Inputs;
            IROpcodeType Type;
        public:
            ScratchBlock * const Block;
            ScratchProcedure * const Procedure = nullptr;
            std::array<ScratchBlock *, 2> BlockLinks = { nullptr, nullptr };
            explicit constexpr IROpcode(ScratchBlock * const SomeBlock, IROpcodeType Type) : Block(SomeBlock), Type(Type) {}
            explicit constexpr IROpcode(ScratchBlock * const SomeBlock, IROpcodeType Type, ScratchProcedure * const ConnectedProcedure) : Block(SomeBlock), Type(Type), Procedure(ConnectedProcedure) {}

            constexpr IROpcodeType GetType(void) const {
                return this->Type;
            }

            constexpr size_t GetNumInputs(void) const {
                return this->Inputs.size();
            }

            constexpr void PushValue(size_t Position, IRValue & IRValue) {
                this->Inputs.emplace(this->Inputs.begin() + Position, std::move(IRValue));
            }
    };

    using IRStack = std::vector<TinyIR::IROpcode>;

    class IRGenerator {
        private:
            IRStack BlockStack;
            size_t BlockCounter;

            void DescendInputs(IROpcode & Opcode);
            IROpcode GenerateOpcode(ScratchBlock * const Block);

            void PrintIR(void);
        public:
            [[nodiscard]]
            const IRStack & GenerateIR(ScratchBlock & Hat);
    };
};