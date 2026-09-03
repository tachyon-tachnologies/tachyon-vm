#include <Tachyon/ExMem.hpp>
#include <Tachyon/Debug.hpp>
#include <Common.hpp>

#include <bit>
#include <cstdint>

/**
 * I'll try to take a similar approach to LuaJIT because LuaJIT is awesome
 * Specifically, from 'lj_mcode.c'
 */

const char * __used TachyonAnchor;

static uintptr_t MinAddress = 0;
static uintptr_t MaxAddress = 0;
static uintptr_t NextAddress = 0;
static size_t PageSize = 0;

static constexpr bool InsideAllocationRange(const void * const Ptr, size_t Size) {
    if (Ptr == nullptr) {
        return false;
    }
    const uintptr_t Address = std::bit_cast<uintptr_t>(Ptr);
    return Address >= MinAddress && Address <= MaxAddress && Size <= MaxAddress - Address;
}

void TachyonAllocator::SetAllocationRange(const void * const Ptr) {
    const uintptr_t AddressAnchor = (std::bit_cast<uintptr_t>(Ptr) + PageSize - 1) & ~(PageSize - 1);
    MaxAddress = AddressAnchor + static_cast<uintptr_t>(INT32_MAX);
    MinAddress = AddressAnchor - (static_cast<uintptr_t>(INT32_MAX) + 1);
    NextAddress = AddressAnchor;
    // DebugInfo("allocator allocation range: %016zx-%016zx\n", MinAddress, MaxAddress);
}

void TachyonAllocator::Init(void) {
#if (defined(_WIN32) || defined(_WIN64))
    SYSTEM_INFO Info;
    GetSystemInfo(&Info);
    PageSize = Info.dwAllocationGranularity;
#else
    PageSize = sysconf(_SC_PAGESIZE);
#endif
}

[[nodiscard]]
void * TachyonAllocator::Alloc(const size_t Size) {
    /* never ever. bad boy */
    if (unlikely(Size == 0 || PageSize == 0)) {
        DebugError("Something isn't right. Page size is this %d.. and requested allocation size is %d... Hmm...\n",
            PageSize, Size
        );
        return nullptr;
    }
    /* first allocation? */
    if (unlikely(MinAddress == 0 || MaxAddress == 0)) {
        TachyonAllocator::SetAllocationRange(&TachyonAnchor);
    }
    /* try allocation */
    for(size_t i = 0; i < 16; i++) {
        void * const Hint = reinterpret_cast<void *>(NextAddress);
        void * Ptr = Tachyon::AllocateJITMemory(Hint, Size);
        if (InsideAllocationRange(Ptr, Size)) {
            const size_t AlignedSize = (Size + PageSize - 1) & ~(PageSize - 1);
            NextAddress = std::bit_cast<uint64_t>(Ptr) + AlignedSize;

            if (NextAddress < MinAddress || NextAddress > MaxAddress) {
                NextAddress = MinAddress;
            }

            return Ptr;
        } else {
            if (unlikely(Ptr != nullptr)) {
                Tachyon::FreeJITMemory(Ptr, Size);
            }
        }
        NextAddress += PageSize;
        if (NextAddress > MaxAddress) {
            NextAddress = MinAddress;
        }
    }
    return nullptr;
}
