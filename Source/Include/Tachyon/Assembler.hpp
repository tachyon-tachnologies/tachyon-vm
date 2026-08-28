#pragma once

#include <Tachyon/Debug.hpp>
#include <Tachyon/ExMem.hpp>
#include <Lib/Hexdump.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <optional>
#include <unordered_map>

#if !defined(__x86_64__)
#error "Only x86 systems are supported!!"
#endif

struct AMD64_Registers {
    uint64_t rbx, rsp, rbp;
    uint64_t r12, r13, r14, r15;
};

struct Tachyon_JITState {
    /**
     * Info contaning entry point and code size
     */
    Tachyon::OutputCodeInfo CodeInfo;

    /**
     * Only used when the Tachyon debugger is enabled
     */
    std::unordered_map<std::string, void *> BlockMap;
    
    /**
     * CPU register state
     */
    AMD64_Registers Registers;    
};

class Label {
    public:
        explicit constexpr Label(uint32_t Id, void * Address) : Id(Id), Start(Address) {}
        constexpr bool operator == (const Label & Other) const {
            return (this->Id == Other.Id && this->Start == Other.Start);
        }
    private:
        const void * const Start;
        const uint32_t Id;
};

class Tachyon_AssemblerBase {
    private:
        std::vector<Label> LabelPool;
        uint32_t LabelIds = 0;
    protected:
        void * CodeBase;
        void * Stack;
        uint8_t * CodePointer;
        size_t CodeSize;
        size_t BytesWritten;

        inline void Write8(uint8_t Byte) {
            *CodePointer++ = Byte;
            BytesWritten++;
        }

        inline void Write16(uint16_t Word) {
            memcpy(CodePointer, &Word, 2);
            CodePointer += 2;
            BytesWritten += 2;
        }

        inline void Write32(uint32_t Dword) {
            memcpy(CodePointer, &Dword, 4);
            CodePointer += 4;
            BytesWritten += 4;
        }

        inline void Write64(uint64_t Qword) {
            memcpy(CodePointer, &Qword, 8);
            CodePointer += 8;
            BytesWritten += 8;
        }
    public:
        Tachyon_AssemblerBase() {
            this->CodeSize = 8192;
            this->BytesWritten = 0;
            /* allocate jit code buffer */
            this->CodeBase = Tachyon::AllocateJITMemory(this->CodeSize);
            TachyonAssert(this->CodeBase != nullptr);

            this->CodePointer = static_cast<uint8_t *>(this->CodeBase);
        }

        ~Tachyon_AssemblerBase() {
            // assuming ownership has been passed
            this->CodeBase = this->CodePointer = nullptr;
            this->CodeSize = 0;
            this->BytesWritten = 0;
        }

        /**
         * @returns Code pointer
         */
        inline void * GetCodePointer(void) {
            return this->CodePointer;
        }

        /**
         * Creates a new label.
         * @returns The newly created label
         */
        constexpr Label CreateLabel(void) {
            return Label(LabelIds++, this->CodePointer);
        }

        constexpr void RegisterLabel(Label & Label) {
            this->LabelPool.emplace_back(Label);
        } 

        /**
         * Makes the JIT code executable and read-only.
         * Developer note: Please for the love of God, free the code memory once it's used!!
         * @return A struct containing the generated code, and the code size (in that order).
         */
        [[nodiscard]]
        inline Tachyon::OutputCodeInfo Commit(void) {
            TachyonAssert(Tachyon::ProtectJITMemory(this->CodeBase, this->CodeSize) == true);
            Tachyon::OutputCode Entry = reinterpret_cast<Tachyon::OutputCode>(this->CodeBase);
            this->CodeDump();
            return {Entry, this->CodeSize};
        }

        constexpr void CodeDump(void) const {
            Debug::Hexdump(this->CodeBase, this->BytesWritten);
        }

};

// NOTE: 1 and 2 use signed displacement
enum ModType : uint8_t {
    NO_DISPLACEMENT,
    BYTE_DISPLACEMENT,
    DWORD_DISPLACEMENT,
    DIRECT_REGISTER
};

/* represents a cpu register */
class GpReg {
    public:
        enum RegisterKind : uint8_t {
            /* 8-bit low-byte registers */
            AL, CL, DL, BL, SPL, BPL, SIL, DIL,
            R8L, R9L, R10L, R11L, R12L, R13L, R14L, R15L,
            /* 8-bit high-byte registers */
            AH, CH, DH, BH,
            /* 16-bit registers */
            AX, CX, DX, BX, SP, BP, SI, DI,
            R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W,
            /* 32-bit registers */
            EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
            R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,
            /* 64-bit registers */
            RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
            R8, R9, R10, R11, R12, R13, R14, R15,
        };

        GpReg() = default;

        explicit constexpr GpReg(const size_t IdValue) {
            this->Value = GpReg::RegisterKind(GpReg::RegisterKind::RAX + (IdValue & 0b111));
            this->RegId = (IdValue & 0b111);
        }
        
        constexpr GpReg(const GpReg::RegisterKind Reg) : Value(Reg) {
            /* cache regid */
            switch (this->Value) {
                /* 8-bit */
                case GpReg::AL...GpReg::BL: {
                    this->RegId = this->Value & 0b111;
                    break;
                }
                case GpReg::AH...GpReg::BH: {
                    this->RegId = (this->Value - GpReg::AH) & 0b111;
                    break;
                }
                /* 8-bit REX extended */
                case GpReg::SPL...GpReg::DIL: {
                    this->RegId = this->Value & 0b111;
                    break;
                }
                case GpReg::R8L...GpReg::R15L: {
                    this->RegId = (this->Value - GpReg::R8L) & 0b111;
                    break;
                }
                /* 16-bit */
                case GpReg::AX...GpReg::DI: {
                    this->RegId = (this->Value - GpReg::AX) & 0b111;
                    break;
                }
                /* 16-bit REX extended */
                case GpReg::R8W...GpReg::R15W: {
                    this->RegId = (this->Value - GpReg::R8W) & 0b111;
                    break;
                }
                /* 32-bit */
                case GpReg::EAX...GpReg::EDI: {
                    this->RegId = (this->Value - GpReg::EAX) & 0b111;
                    break;
                }
                /* 32-bit REX extended */
                case GpReg::R8D...GpReg::R15D: {
                    this->RegId = (this->Value - GpReg::R8D) & 0b111;
                    break;
                }
                /* 64-bit */
                case GpReg::RAX...GpReg::RDI: {
                    this->RegId = ((this->Value) - GpReg::RAX) & 0b111;
                    break;
                }
                /* 64-bit REX extended */
                case GpReg::R8...GpReg::R15: {
                    this->RegId = (this->Value - GpReg::R8) & 0b111;
                    break;
                }
                default: {
                    // abort this like a child
                    TachyonAbort("Invalid register value %d. Aborting...", Reg);
                    __builtin_unreachable();
                }
            }

        }
        explicit operator bool() const = delete;

        constexpr operator GpReg::RegisterKind() const {
            return this->Value;
        }

        constexpr bool operator == (const GpReg::RegisterKind & Reg) const {
            return this->Value == Reg;
        }

        constexpr bool operator != (const GpReg::RegisterKind & Reg) const {
            return this->Value != Reg;
        }

        constexpr GpReg::RegisterKind GetRawValue(void) const {
            return this->Value;
        }

        constexpr uint8_t AsRegID(void) const {
            return this->RegId;
        }
        /* register bit widths */
        constexpr bool Is64bit(void) const {
            switch (this->Value) {
                case GpReg::RAX...GpReg::R15: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }
        constexpr bool Is32bit(void) const {
            switch (this->Value) {
                case GpReg::EAX...GpReg::R15D: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }
        constexpr bool Is16bit(void) const {
            switch (this->Value) {
                case GpReg::AX...GpReg::R15W: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }
        constexpr bool Is8bit(void) const {
            switch (this->Value) {
                case GpReg::AL...GpReg::R15L: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }
        /* register requires REX? */
        constexpr bool RequiresREX(void) const {
            switch (this->Value) {
                /* fallthrough stack */
                case GpReg::SPL...GpReg::DIL:
                case GpReg::R8L...GpReg::R15L:
                case GpReg::R8W...GpReg::R15W:
                case GpReg::R8D...GpReg::R15D:
                case GpReg::R8...GpReg::R15: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* AL, AH, AX, EAX, RAX */
        constexpr bool IsAX(void) const {
            switch (this->RegId) {
                case GpReg::AL:
                case GpReg::AH:
                case GpReg::AX:
                case GpReg::EAX:
                case GpReg::RAX: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* CL, CH, CX, ECX, RCX */
        constexpr bool IsCX(void) const {
            switch (this->Value) {
                case GpReg::CL:
                case GpReg::CH:
                case GpReg::CX:
                case GpReg::ECX:
                case GpReg::RCX: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* DL, DH, DX, EDX, RDX */
        constexpr bool IsDX(void) const {
            switch (this->Value) {
                case GpReg::DL:
                case GpReg::DH:
                case GpReg::DX:
                case GpReg::EDX:
                case GpReg::RDX: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* BL, BH, BX, EBX, RBX */
        constexpr bool IsBX(void) const {
            switch (this->Value) {
                case GpReg::BL:
                case GpReg::BH:
                case GpReg::BX:
                case GpReg::EBX:
                case GpReg::RBX: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* SIL, SI, ESI, RSI */
        constexpr bool IsSI(void) const {
            switch (this->Value) {
                case GpReg::SIL:
                case GpReg::SI:
                case GpReg::ESI:
                case GpReg::RSI: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* DIL, DI, EDI, RDI */
        constexpr bool IsDI(void) const {
            switch (this->Value) {
                case GpReg::DIL:
                case GpReg::DI:
                case GpReg::EDI:
                case GpReg::RDI: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* SPL, SP, ESP, RSP */
        constexpr bool IsSP(void) const {
            switch (this->Value) {
                case GpReg::SPL:
                case GpReg::SP:
                case GpReg::ESP:
                case GpReg::RSP: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* BPL, BP, EBP, RBP */
        constexpr bool IsBP(void) const {
            switch (this->Value) {
                case GpReg::BPL:
                case GpReg::BP:
                case GpReg::EBP:
                case GpReg::RBP: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* R8L, R8W, R8D, R8 */
        constexpr bool IsR8(void) const {
            switch (this->Value) {
                case GpReg::R8L:
                case GpReg::R8W:
                case GpReg::R8D:
                case GpReg::R8: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* R9L, R9W, R9D, R9 */
        constexpr bool IsR9(void) const {
            switch (this->Value) {
                case GpReg::R9L:
                case GpReg::R9W:
                case GpReg::R9D:
                case GpReg::R9: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* R10L, R10W, R10D, R10 */
        constexpr bool IsR10(void) const {
            switch (this->Value) {
                case GpReg::R10L:
                case GpReg::R10W:
                case GpReg::R10D:
                case GpReg::R10: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* R11L, R11W, R11D, R11 */
        constexpr bool IsR11(void) const {
            switch (this->Value) {
                case GpReg::R11L:
                case GpReg::R11W:
                case GpReg::R11D:
                case GpReg::R11: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* R12L, R12W, R12D, R12 */
        constexpr bool IsR12(void) const {
            switch (this->Value) {
                case GpReg::R12L:
                case GpReg::R12W:
                case GpReg::R12D:
                case GpReg::R12: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* R13L, R13W, R13D, R13 */
        constexpr bool IsR13(void) const {
            switch (this->Value) {
                case GpReg::R13L:
                case GpReg::R13W:
                case GpReg::R13D:
                case GpReg::R13: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* R14L, R14W, R14D, R14 */
        constexpr bool IsR14(void) const {
            switch (this->Value) {
                case GpReg::R14L:
                case GpReg::R14W:
                case GpReg::R14D:
                case GpReg::R14: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        /* R15L, R15W, R15D, R15 */
        constexpr bool IsR15(void) const {
            switch (this->Value) {
                case GpReg::R15L:
                case GpReg::R15W:
                case GpReg::R15D:
                case GpReg::R15: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }
        constexpr bool RequiresOpsizePrefix(void) const {
            return this->Is16bit();
        }
        
        constexpr bool RequiresAddrsizePrefix(void) const {
            return this->Is32bit();
        }

        constexpr bool ShouldMemAccessThroughRM(void) const {
            switch(this->RegId) {
                case 0b101:
                case 0b100: {
                    return false;
                }
                default: {
                    return true;
                }
            }
        }

    private:
        bool UseREX;
        RegisterKind Value;
        uint8_t RegId;
};

using Disp = std::variant<int32_t, int8_t>;
using GpRegDisp = std::pair<GpReg, Disp>;

/* represents a form of memory access */
class Mem {
    public:
        struct Sib {
            uint8_t Scale;
            GpReg Index;
            GpReg Base;
            std::optional<Disp> Displacement;
        };

        enum MemType : uint8_t {
            MEM_SIB,
            MEM_REG,
            MEM_REG_DISP,
        };

        constexpr Mem(const uint8_t S, const GpReg I, const GpReg B) : AccessType(Mem::MEM_SIB) {
            this->Value = (struct Sib){
                S,
                I,
                B,
                std::nullopt
            };
        }

        constexpr Mem(const uint8_t S, const GpReg I, const GpReg B, const int32_t Displacement) : AccessType(Mem::MEM_SIB) {
            this->Value = (struct Sib){
                S,
                I,
                B,
                Disp(Displacement)
            };
        }

        // for displacement only, make S=0, I=GpReg::SP, B=GpReg::BP
        constexpr Mem(const uint8_t S, const GpReg I, const GpReg B, const int8_t Displacement) : AccessType(Mem::MEM_SIB) {
            this->Value = (struct Sib){
                S,
                I,
                B,
                Disp(Displacement)
            };
        }

        constexpr Mem(const GpReg Register) : AccessType(Mem::MEM_REG) {
            this->Value = Register;
        }

        constexpr Mem(const GpReg Register, const int32_t Displacement) : AccessType(Mem::MEM_REG_DISP) {
            this->Value = GpRegDisp(Register, Displacement);
        }

        constexpr Mem(const GpReg Register, const int8_t Displacement) : AccessType(Mem::MEM_REG_DISP) {
            this->Value = GpRegDisp(Register, Displacement);
        }

        constexpr bool IsSIB(void) const {
            return std::holds_alternative<struct Sib>(this->Value);
        }

        constexpr bool IsRegister(void) const {
            return std::holds_alternative<GpReg>(this->Value);
        }

        constexpr bool IsRegisterDisp(void) const {
            return std::holds_alternative<GpRegDisp>(this->Value);
        }

        constexpr uint8_t GetSIB(void) const {
            TachyonAssertMsg(this->IsSIB() == true, "Invalid memory access type! Expected SIB type.\n");
            const struct Sib & Sib = std::get<struct Sib>(this->Value);
            return (Sib.Scale & 0b11) << 6 |
                   (Sib.Index.AsRegID() & 0b111) << 3 |
                   (Sib.Base.AsRegID() & 0b111);
        }

        /* assumes that this is 100% a SIB type */
        constexpr Sib & GetSIBStruct(void) {
            return std::get<Mem::Sib>(this->Value);
        }

        constexpr Disp GetSIBDisplacement(void) const {
            TachyonAssertMsg(this->IsSIB() == true, "Invalid memory access type! Expected SIB type.\n");
            const struct Sib & Sib = std::get<struct Sib>(this->Value);
            return Sib.Displacement.value_or(Disp(0));
        }

        constexpr const GpReg GetRegister(void) const {
            TachyonAssertMsg(this->IsRegister() == true, "Invalid memory access type! Expected register type.\n");
            return std::get<GpReg>(this->Value);
        }

        constexpr const GpRegDisp GetRegisterDisp(void) const {
            TachyonAssertMsg(this->IsRegisterDisp() == true, "Invalid memory access type! Expected register with displacement type.\n");
            return std::get<GpRegDisp>(this->Value);
        }
        
        constexpr MemType GetType(void) const {
            return this->AccessType;
        }

    private:
        const MemType AccessType;
        std::variant<struct Sib, GpReg, GpRegDisp> Value;
};

/**
 * Tachyon's AMD64 (x86_64) assembler.
 */
class Tachyon_AssemblerAMD64 : public Tachyon_AssemblerBase {
    // https://wiki.osdev.org/X86-64_Instruction_Encoding
    private:
        /*
            REX
        */
        void EmitREX(const bool Opsize64, const bool R, const bool X, const bool B);
        uint8_t InitRegsREX(const GpReg & Reg1, const GpReg & Reg2);
        void SetREX_Opsize(uint8_t & REX, const bool Opsize64);
        void SetREX_RegExtension(uint8_t & REX, const bool R);
        void SetREX_SIBExtension(uint8_t & REX, const bool X);
        void SetREX_BaseExtension(uint8_t & REX, const bool B);
        /*
            ModR/M
        */
        // op-size prefix (32 -> 16)
        void EmitOpsizePrefix(void);
        // addr-size prefix (64 -> 32)
        void EmitAddrsizePrefix(void);
        void SetREG(uint8_t & ModRM, const uint8_t Reg);
        void SetRM(uint8_t & ModRM, const uint8_t RM);
        void SetModRM_Access(uint8_t & ModRM, const ModType Access);
        void SetModRM_Register(uint8_t & ModRM, const GpReg & Register, const bool IsRM, const bool ShouldEmitREX = true);
    public:
        /*
            Commonly used functions
        */

        void EmitMainPrologue(Tachyon_JITState & State);
        void EmitMainEpilogue(void);

        void EmitFunctionPrologue(void);
        void EmitFunctionEpilogue(void);
        
        /*
            Arithmetic
        */

        void Xor(const GpReg & Operand1, const GpReg & Operand2);

        void Add(const GpReg & Reg, uint8_t Imm);
        void Sub(const GpReg & Reg, uint8_t Imm);

        void Mov(const GpReg & Dest, const uint8_t Imm);
        void Mov(const GpReg & Dest, const uint16_t Imm);
        void Mov(const GpReg & Dest, const uint32_t Imm);
        void Mov(const GpReg & Dest, const uint64_t Imm);

        void Mov(const GpReg & Dest, const GpReg & Src);

        void Mov(const Mem & Dest, const GpReg & Src);

        void Push(const GpReg & Register);
        void Push(uint8_t Imm);
        void Push(uint32_t Imm);

        void Pop(const GpReg & Register);

        void RelCall(const int32_t Disp32);
        void RelCall(const int16_t Disp16);
        void CallFunction(const void * const FunctionPtr);
        void IndirectMemoryCall(const GpReg & Register);
        void IndirectRegisterCall(const GpReg & Register);

        void Ret(void);
        void Ret(const uint16_t Bytes);

        /**
         * Misc.
         */
        void Leave(void) {
            this->Write8(0xC9);
        }
        void Nop(void) {
            this->Write8(0x90);
        }
};

#if defined(__x86_64__)
using TachyonAssembler = Tachyon_AssemblerAMD64;
#endif
