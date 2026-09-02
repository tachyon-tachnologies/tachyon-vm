#pragma once

#if (defined(_WIN32) || defined(_WIN64))
#include <Windows.h>
#endif
#include <Tachyon/Debug.hpp>

#include <unistd.h>
#include <stddef.h>
#include <cstdint>
#include <bit>

class TachyonAllocator {
    private:
        uint64_t MinAddress = 0;
        uint64_t MaxAddress = 0;
        void * Hint = nullptr;

        size_t PageSize;

        void SetAllocationRange(const void * const Ptr);
        constexpr bool InsideAllocationRange(const void * const Ptr) {
            const uint64_t Address = std::bit_cast<uint64_t>(Ptr);
            return (Address >= this->MinAddress && Address <= this->MaxAddress);
        }
    public:
        TachyonAllocator() {
            this->PageSize = sysconf(_SC_PAGESIZE);
        }
        [[nodiscard]]
        void * Alloc(const size_t Size);
};

namespace Tachyon {
    /**
     * @returns Beginning of JIT buffer region
     */
    void * GetJITRegionStart(void);
    /**
     * Reads /proc/self/maps to determine where to place the JIT code buffer
     * @returns True if successful, false if otherwise
     */
    bool ReadProcMaps(void);

	/**
	 * Allocates JIT memory with read-write permissions
	 * @param Size JIT memory size
	 * @returns A pointer to the allocated memory
	 */
	void * AllocateJITMemory(void * Hint, const size_t Size);
	
	/**
	 * Frees JIT memory
	 * @param Base JIT memory base/address
	 * @param Size JIT memory size
	 */
	void FreeJITMemory(void * const Base, const size_t Size);

	/**
	 * Protect JIT memory with read-execute permissions
	 * @param Base JIT memory base/address
	 * @param Size JIT memory size
	 * @returns True if successful, false if otherwise.
	 */
	bool ProtectJITMemory(void * const Base, const size_t Size);

	/**
	 * Flushes instruction cache in a specific address
	 * @param Base JIT memory base/address
	 * @param Size JIT memory size 
	 */
	inline void PrepareCPUCache(void * const Base, const size_t Size) {
#if (defined(_WIN32) || defined(_WIN64))
		bool Result;
		HANDLE ProcHandle = GetCurrentProcess();
		Result = FlushInstructionCache(ProcHandle, Base, Size);
		TachyonAssertMsg(Result == true, "Failed to flush instruction cache\n");
#else
		__builtin___clear_cache((char *)Base, (char *)Base + Size);
#endif
	}

	using OutputCode = int (*)(void);

	struct OutputCodeInfo {
		/**
		 * JIT entry point
		 */
		OutputCode CodeEntry;

		/**
		 * JIT allocated code size
		 */
		size_t CodeSize;

		inline void FreeMemory(void) {
			if (likely(this->CodeEntry)) {
				Tachyon::FreeJITMemory(reinterpret_cast<void *>(this->CodeEntry), this->CodeSize);
				this->CodeEntry = nullptr;
			}
		}
	};
};
