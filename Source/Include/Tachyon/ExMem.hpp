#pragma once

#if (defined(_WIN32) || defined(_WIN64))
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <Tachyon/Debug.hpp>

#include <stddef.h>
#include <cstdint>
#include <bit>

namespace TachyonAllocator {
	void SetAllocationRange(const void * const Ptr);

	[[nodiscard]]
	void * Alloc(const size_t Size);

	void Init(void);
};

namespace Tachyon {
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
