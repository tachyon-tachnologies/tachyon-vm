#pragma once

#if (defined(_WIN32) || defined(_WIN64))
#include <Windows.h>
#endif
#include <Tachyon/Debug.hpp>
#include <stddef.h>

namespace Tachyon {
	/**
	 * Allocates stack memory with read-only, NX permissions
	 * @returns A pointer to the allocated memory
	 */
	void * AllocateStack(void);

	/**
	 * Frees stack memory
	 * @param Base JIT stack base/address
	 */
	void FreeStack(void * Base);

	/**
	 * Allocates JIT memory with read-write permissions
	 * @param Size JIT memory size
	 * @returns A pointer to the allocated memory
	 */
	void * AllocateJITMemory(const size_t Size);
	
	/**
	 * Frees JIT memory
	 * @param Base JIT memory base/address
	 * @param Size JIT memory size
	 */
	void FreeJITMemory(void * Base, const size_t Size);

	/**
	 * Protect JIT memory with read-execute permissions
	 * @param Base JIT memory base/address
	 * @param Size JIT memory size
	 * @returns True if successful, false if otherwise.
	 */
	bool ProtectJITMemory(void * Base, const size_t Size);

	/**
	 * Flushes instruction cache in a specific address
	 * @param Base JIT memory base/address
	 * @param Size JIT memory size 
	 */
	inline void PrepareCPUCache(void * Base, const size_t Size) {
#if (defined(_WIN32) || defined(_WIN64))
		bool Result;
		HANDLE ProcHandle = GetCurrentProcess();
		Result = FlushInstructionCache(ProcHandle, Base, Size);
		TachyonAssertMsg(Result == true, "Failed to flush instruction cache\n");
#else
		__builtin___clear_cache((char *)Base, (char *)Base + Size);
#endif
	}
};