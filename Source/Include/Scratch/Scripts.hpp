#pragma once

#include <vector>
#include <string>
#include <variant>
#include <unordered_map>

#include <Scratch/Status.hpp>
#include <Tachyon/Assembler.hpp>
#include <Lib/NanBox.hpp>

using namespace NanBox;

/* control flags. any bits that arent here are reserved */

#define SCRIPT_INSIDE_PROCEDURE         (1 << 0)
#define SCRIPT_INVALIDATE_BLOCK         (1 << 1)
#define SCRIPT_SHOULD_STAY              (1 << 2)

namespace Scratch {
    class ScratchSprite;
    class ScratchBlock;

    /**
     * Stack information for script.
     */
    struct Script_StackFrame {
        std::string ReturnId;
        std::string RepeatId;
        std::variant<double, ScratchBlock *> RepeatCondition;
        bool InsideProcedure;
    };

    using ProcedureBindings = std::unordered_map<std::string, BoxedValue>;

    /**
     * Contains the information of a script.
     */
    class ScratchScript {
        public:
            std::string FirstBlockId;
            std::string CurrentBlockId;
            ScratchSprite * Sprite;
            ScratchStatus CurrentStatus;
            Tachyon_JITState JITState = {};
            uint8_t ControlFlags;

            inline void __hot SetControlFlag(const uint8_t Flag) {
                this->ControlFlags |= Flag;
            }

            inline void __hot UnsetControlFlag(const uint8_t Flag) {
                this->ControlFlags &= ~Flag;
            }

            constexpr bool __hot GetControlFlag(const uint8_t Flag) {
                return (this->ControlFlags & Flag);
            }

            inline void __hot Return(void) {
                TachyonAssertMsg(this->ReturnStack.empty() == false, "Stack underflow!\n");

                const Script_StackFrame & CurrentStackFrame = this->ReturnStack.back();
                /* restore procedure control flag */
                this->ControlFlags &= ~(SCRIPT_INSIDE_PROCEDURE);
                this->ControlFlags |= CurrentStackFrame.InsideProcedure;

                this->CurrentBlockId = CurrentStackFrame.ReturnId;
                this->ReturnStack.pop_back();
            }

            inline void __hot UnbindParameters(void) {
                TachyonAssertMsg(this->ParamBindings.empty() == false, "Parameter bindings are empty??\n");
                this->ParamBindings.pop_back();
            }

            inline ScratchStatus __hot RecursiveReturn(void) {
                while(likely(this->CurrentBlockId.empty() == true)) {
                    if (this->GetControlFlag(SCRIPT_INSIDE_PROCEDURE) == false) {
                        return ScratchStatus::SCRATCH_END;
                    }
                    this->Return();
                    this->UnbindParameters();
                }
                return ScratchStatus::SCRATCH_NEXT;
            }

            inline void StackPush(const Script_StackFrame Frame) {
                this->ReturnStack.push_back(Frame);
            }

            inline std::vector<ProcedureBindings> & GetParameterBindings(void) {
                return this->ParamBindings;
            }

            ScratchScript() = default;
            ScratchScript(const std::string CurrentBlockKey, ScratchSprite * Owner) : FirstBlockId(CurrentBlockKey), CurrentBlockId(CurrentBlockKey), Sprite(Owner) {
                this->CurrentStatus = ScratchStatus::SCRATCH_END;
                this->ControlFlags = 0;
                this->ParamBindings = {};
                this->ReturnStack = {};
                this->ParamBindings.reserve(4);
                this->ReturnStack.reserve(32);
            }
        private:
            std::vector<Script_StackFrame> ReturnStack;
            std::vector<ProcedureBindings> ParamBindings;
    };

    static constexpr bool __hot ShouldRepeatConditionally(const Script_StackFrame & Frame) {
        return std::holds_alternative<ScratchBlock *>(Frame.RepeatCondition);
    }
};