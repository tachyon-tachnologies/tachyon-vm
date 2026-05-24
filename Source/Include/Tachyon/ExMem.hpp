#pragma once

#if (defined(_WIN32) || defined(_WIN64))
#include <Windows.h>
#endif
#include <Tachyon/Debug.hpp>
#include <stddef.h>

namespace Tachyon {
	/* allocates writable memory (R/W) */
	void * AllocateCodeMemory(const size_t Size);
	/* frees code memory */
	void FreeCodeMemory(void * Base, const size_t Size);
	/* sets the page permissions to executable and read-only (R/X) */
	bool ProtectCodeMemory(void * Base, const size_t Size);
	/* flush instruction cache */
	inline void PrepareCPUCache(void * Base, const size_t Size) {
		bool Result = true;
#if (defined(_WIN32) || defined(_WIN64))
		HANDLE ProcHandle = GetCurrentProcess();
		Result = FlushInstructionCache(ProcHandle, Base, Size);
#else
		__builtin___clear_cache((char *)Base, (char *)Base + Size);
#endif
		TachyonAssertMsg(Result == true, "Failed to flush instruction cache");
	}
};