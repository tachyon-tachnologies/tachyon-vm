#pragma once

#include <Lib/Hexdump.hpp>
#include <Tachyon/Debug.hpp>
#include <Tachyon/ExMem.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <variant>
#include <optional>

using OutputCode = int (*)(void);

class Tachyon_EncoderImpl {
    protected:
        void * CodeBase;
        uint8_t * CodePointer;
        size_t CodeSize;
        size_t BytesWritten;

        void Write8(uint8_t Byte) {
            *CodePointer++ = Byte;
            BytesWritten++;
        }
        void Write16(uint16_t Word) {
            memcpy(CodePointer, &Word, 2);
            CodePointer += 2;
            BytesWritten += 2;
        }
        void Write32(uint32_t Dword) {
            memcpy(CodePointer, &Dword, 4);
            CodePointer += 4;
            BytesWritten += 4;
        }
        void Write64(uint64_t Qword) {
            memcpy(CodePointer, &Qword, 8);
            CodePointer += 8;
            BytesWritten += 8;
        }
    public:
        Tachyon_EncoderImpl() {
            CodeSize = 8192;
            BytesWritten = 0;
            CodeBase = Tachyon::AllocateCodeMemory(CodeSize);
            TachyonAssert(CodeBase != nullptr);
            CodePointer = static_cast<uint8_t *>(CodeBase);
            DebugInfo("JIT compiler allocated code range: [%p - %p]\n", CodeBase, CodePointer + CodeSize);
        }

        ~Tachyon_EncoderImpl() {
            Tachyon::FreeCodeMemory(CodeBase, CodeSize);
            CodeBase = CodePointer = nullptr;
            CodeSize = 0;
            BytesWritten = 0;
        }

        inline OutputCode MakeExecutable(void) {
            TachyonAssert(Tachyon::ProtectCodeMemory(CodeBase, CodeSize) == true);
            Tachyon::PrepareCPUCache(CodeBase, CodeSize);
            return (OutputCode)CodeBase;
        }

        inline void CodeDump(void) const {
            Debug::Hexdump(this->CodeBase, BytesWritten);
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
            REG_AL, REG_CL, REG_DL, REG_BL, REG_SPL, REG_BPL, REG_SIL, REG_DIL,
            REG_R8L, REG_R9L, REG_R10L, REG_R11L, REG_R12L, REG_R13L, REG_R14L, REG_R15L,
            /* 8-bit high-byte registers */
            REG_AH, REG_CH, REG_DH, REG_BH,
            /* 16-bit registers */
            REG_AX, REG_CX, REG_DX, REG_BX, REG_SP, REG_BP, REG_SI, REG_DI,
            REG_R8W, REG_R9W, REG_R10W, REG_R11W, REG_R12W, REG_R13W, REG_R14W, REG_R15W,
            /* 32-bit registers */
            REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_EBP, REG_ESI, REG_EDI,
            REG_R8D, REG_R9D, REG_R10D, REG_R11D, REG_R12D, REG_R13D, REG_R14D, REG_R15D,
            /* 64-bit registers */
            REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
            REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15,
        };

        GpReg() = default;
        constexpr GpReg(const GpReg::RegisterKind Reg) : Value(Reg) {
            /* cache regid */
            switch (this->Value) {
                /* 8-bit */
                case GpReg::REG_AL...GpReg::REG_BL: {
                    this->RegId = this->Value & 0b111;
                    break;
                }
                case GpReg::REG_AH...GpReg::REG_BH: {
                    this->RegId = (this->Value - GpReg::REG_AH) & 0b111;
                    break;
                }
                /* 8-bit REX extended */
                case GpReg::REG_SPL...GpReg::REG_DIL: {
                    this->RegId = this->Value & 0b111;
                    break;
                }
                case GpReg::REG_R8L...GpReg::REG_R15L: {
                    this->RegId = (this->Value - GpReg::REG_R8L) & 0b111;
                }
                /* 16-bit */
                case GpReg::REG_AX...GpReg::REG_DI: {
                    this->RegId = (this->Value - GpReg::REG_AX) & 0b111;
                    break;
                }
                /* 16-bit REX extended */
                case GpReg::REG_R8W...GpReg::REG_R15W: {
                    this->RegId = (this->Value - GpReg::REG_R8W) & 0b111;
                    break;
                }
                /* 32-bit */
                case GpReg::REG_EAX...GpReg::REG_EDI: {
                    this->RegId = (this->Value - GpReg::REG_EAX) & 0b111;
                    break;
                }
                /* 32-bit REX extended */
                case GpReg::REG_R8D...GpReg::REG_R15D: {
                    this->RegId = (this->Value - GpReg::REG_R8D) & 0b111;
                    break;
                }
                /* 64-bit */
                case GpReg::REG_RAX...GpReg::REG_RDI: {
                    this->RegId = ((this->Value) - GpReg::REG_RAX) & 0b111;
                    break;
                }
                /* 64-bit REX extended */
                case GpReg::REG_R8...GpReg::REG_R15: {
                    this->RegId = (this->Value - GpReg::REG_R8) & 0b111;
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

        constexpr bool operator == (const GpReg::RegisterKind Reg) const {
            return this->Value == Reg;
        }

        constexpr bool operator != (const GpReg::RegisterKind Reg) const {
            return this->Value != Reg;
        }

        constexpr uint8_t AsRegID(void) const {
            return this->RegId;
        }
        /* register bit widths */
        constexpr bool Is64bit(void) const {
            switch (this->Value) {
                case GpReg::REG_RAX...GpReg::REG_R15: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }
        constexpr bool Is32bit(void) const {
            switch (this->Value) {
                case GpReg::REG_EAX...GpReg::REG_R15D: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }
        constexpr bool Is16bit(void) const {
            switch (this->Value) {
                case GpReg::REG_AX...GpReg::REG_R15W: {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }
        constexpr bool Is8bit(void) const {
            switch (this->Value) {
                case GpReg::REG_AL...GpReg::REG_R15L: {
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
                case GpReg::REG_SPL...GpReg::REG_DIL:
                case GpReg::REG_R8L...GpReg::REG_R15L:
                case GpReg::REG_R8W...GpReg::REG_R15W:
                case GpReg::REG_R8D...GpReg::REG_R15D:
                case GpReg::REG_R8...GpReg::REG_R15: {
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
                case GpReg::REG_AL:
                case GpReg::REG_AH:
                case GpReg::REG_AX:
                case GpReg::REG_EAX:
                case GpReg::REG_RAX: {
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
                case GpReg::REG_CL:
                case GpReg::REG_CH:
                case GpReg::REG_CX:
                case GpReg::REG_ECX:
                case GpReg::REG_RCX: {
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
                case GpReg::REG_DL:
                case GpReg::REG_DH:
                case GpReg::REG_DX:
                case GpReg::REG_EDX:
                case GpReg::REG_RDX: {
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
                case GpReg::REG_BL:
                case GpReg::REG_BH:
                case GpReg::REG_BX:
                case GpReg::REG_EBX:
                case GpReg::REG_RBX: {
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
                case GpReg::REG_SIL:
                case GpReg::REG_SI:
                case GpReg::REG_ESI:
                case GpReg::REG_RSI: {
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
                case GpReg::REG_DIL:
                case GpReg::REG_DI:
                case GpReg::REG_EDI:
                case GpReg::REG_RDI: {
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
                case GpReg::REG_SPL:
                case GpReg::REG_SP:
                case GpReg::REG_ESP:
                case GpReg::REG_RSP: {
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
                case GpReg::REG_BPL:
                case GpReg::REG_BP:
                case GpReg::REG_EBP:
                case GpReg::REG_RBP: {
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
                case GpReg::REG_R8L:
                case GpReg::REG_R8W:
                case GpReg::REG_R8D:
                case GpReg::REG_R8: {
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
                case GpReg::REG_R9L:
                case GpReg::REG_R9W:
                case GpReg::REG_R9D:
                case GpReg::REG_R9: {
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
                case GpReg::REG_R10L:
                case GpReg::REG_R10W:
                case GpReg::REG_R10D:
                case GpReg::REG_R10: {
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
                case GpReg::REG_R11L:
                case GpReg::REG_R11W:
                case GpReg::REG_R11D:
                case GpReg::REG_R11: {
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
                case GpReg::REG_R12L:
                case GpReg::REG_R12W:
                case GpReg::REG_R12D:
                case GpReg::REG_R12: {
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
                case GpReg::REG_R13L:
                case GpReg::REG_R13W:
                case GpReg::REG_R13D:
                case GpReg::REG_R13: {
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
                case GpReg::REG_R14L:
                case GpReg::REG_R14W:
                case GpReg::REG_R14D:
                case GpReg::REG_R14: {
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
                case GpReg::REG_R15L:
                case GpReg::REG_R15W:
                case GpReg::REG_R15D:
                case GpReg::REG_R15: {
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
    private:
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

        Mem(const uint8_t S, const GpReg I, const GpReg B) {
            this->Value = (struct Sib){
                S,
                I,
                B,
                std::nullopt
            };
            this->AccessType = Mem::MEM_SIB;
        }

        Mem(const uint8_t S, const GpReg I, const GpReg B, const int32_t Displacement) {
            this->Value = (struct Sib){
                S,
                I,
                B,
                Disp(Displacement)
            };
            this->AccessType = Mem::MEM_SIB;
        }

        // for displacement only, make S=0, I=GpReg::REG_SP, B=GpReg::REG_BP
        Mem(const uint8_t S, const GpReg I, const GpReg B, const int8_t Displacement) {
            this->Value = (struct Sib){
                S,
                I,
                B,
                Disp(Displacement)
            };
            this->AccessType = Mem::MEM_SIB;
        }

        Mem(GpReg Register) {
            this->Value = Register;
            this->AccessType = Mem::MEM_REG;
        }

        Mem(GpReg Register, int32_t Displacement) {
            this->Value = GpRegDisp(Register, Displacement);
            this->AccessType = Mem::MEM_REG_DISP;
        }

        Mem(GpReg Register, int8_t Displacement) {
            this->Value = GpRegDisp(Register, Displacement);
            this->AccessType = Mem::MEM_REG_DISP;
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
            TachyonAssertMsg(this->IsSIB() == true, "Invalid memory access type! Expected SIB type.");
            const struct Sib & Sib = std::get<struct Sib>(this->Value);
            return (Sib.Scale & 0b11) << 6 |
                   (Sib.Index.AsRegID() & 0b111) << 3 |
                   (Sib.Base.AsRegID() & 0b111);
        }

        /* assumes that this is 100% a SIB type */
        constexpr Mem::Sib & GetSIBStruct(void) {
            return std::get<Mem::Sib>(this->Value);
        }

        constexpr Disp GetSIBDisplacement(void) const {
            TachyonAssertMsg(this->IsSIB() == true, "Invalid memory access type! Expected SIB type.");
            const struct Sib& Sib = std::get<struct Sib>(this->Value);
            return Sib.Displacement.value_or(Disp(0));
        }

        constexpr const GpReg GetRegister(void) const {
            TachyonAssertMsg(this->IsRegister() == true, "Invalid memory access type! Expected register type.");
            return std::get<GpReg>(this->Value);
        }

        constexpr const GpRegDisp GetRegisterDisp(void) const {
            TachyonAssertMsg(this->IsRegisterDisp() == true, "Invalid memory access type! Expected register with displacement type.");
            return std::get<GpRegDisp>(this->Value);
        }
        
        constexpr const Mem::MemType GetType(void) const {
            return this->AccessType;
        }

    private:
        MemType AccessType;
        std::variant<struct Sib, GpReg, GpRegDisp> Value;
};

class Tachyon_AMD64Encoder : public Tachyon_EncoderImpl {
    // https://wiki.osdev.org/X86-64_Instruction_Encoding
    private:
        inline void EmitREX(const bool Opsize64, const bool R, const bool X, const bool B);

        // op-size prefix (32 -> 16)
        inline void EmitOpsizePrefix(void);

        // addr-size prefix (64 -> 32)
        inline void EmitAddrsizePrefix(void);

        inline void SetREG(uint8_t & ModRM, const uint8_t Reg);

        inline void SetRM(uint8_t & ModRM, const uint8_t RM);

        inline void SetModRM_Access(uint8_t & ModRM, const ModType Access);

        inline void SetModRM_Register(uint8_t & ModRM, GpReg Register, const bool IsRM, const bool ShouldEmitREX = true);
    public:
        void Mov(const GpReg Dest, const uint8_t Imm);
        void Mov(const GpReg Dest, const uint16_t Imm);
        void Mov(const GpReg Dest, const uint32_t Imm);
        void Mov(const GpReg Dest, const uint64_t Imm);

        void Mov(const Mem Dest, const GpReg Src);

        void Ret(void);
        void Ret(uint16_t Bytes);
};