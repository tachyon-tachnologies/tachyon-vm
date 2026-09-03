#include <Tachyon/ExMem.hpp>
#include <Tachyon/Debug.hpp>
#include <Common.hpp>

#include <bit>
#include <cstdint>

#if (defined(_WIN32) || defined(_WIN64))
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

[[nodiscard]]
void * Tachyon::AllocateJITMemory(void * const Hint, const size_t Size) {
#if (defined(_WIN32) || defined(_WIN64))
	return VirtualAlloc(Hint, Size, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
#else
	void * Addr = mmap(Hint, Size, (PROT_WRITE | PROT_READ), (MAP_PRIVATE | MAP_ANON), -1, 0);
	return (Addr == MAP_FAILED) ? nullptr : Addr;
#endif
}

void Tachyon::FreeJITMemory(void * const Base, [[maybe_unused]] const size_t Size) {
	if (unlikely(Base == nullptr)) {
		return;
	}
#if (defined(_WIN32) || defined(_WIN64))
	bool rc = VirtualFree(Base, 0, MEM_RELEASE);
	TachyonAssertMsg(rc == true, "Failed to free JIT code memory: 0x%08zx\n", GetLastError());
#else
	TachyonAssertMsg(munmap(Base, Size) != -1, "Failed to free JIT code memory\n");
#endif
}

[[nodiscard]]
bool Tachyon::ProtectJITMemory(void * const Base, const size_t Size) {
	if (unlikely(Base == nullptr)) {
		return false;
	}
#if (defined(_WIN32) || defined(_WIN64))
	DWORD PreviousProtection;
	bool rc = VirtualProtect(Base, Size, PAGE_EXECUTE_READ, &PreviousProtection);
	return rc;
#else
	return mprotect(Base, Size, (PROT_EXEC | PROT_READ)) != -1;
#endif
}