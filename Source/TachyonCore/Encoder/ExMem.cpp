#include <Tachyon/ExMem.hpp>
#include <Tachyon/Debug.hpp>
#include <Common.hpp>
#include <stddef.h>

#if (defined(_WIN32) || defined(_WIN64))
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

void * Tachyon::AllocateCodeMemory(const size_t Size) {
#if (defined(_WIN32) || defined(_WIN64))
	return VirtualAlloc(nullptr, Size, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
#else
	void * Addr = mmap(0, Size, (PROT_WRITE | PROT_READ), (MAP_PRIVATE | MAP_ANON), -1, 0);
	return (Addr == MAP_FAILED) ? NULL : Addr;
#endif
}

void Tachyon::FreeCodeMemory(void * Base, [[maybe_unused]] const size_t Size) {
	if (Base == nullptr) {
		return;
	}
#if (defined(_WIN32) || defined(_WIN64))
	bool rc = VirtualFree(Base, 0, MEM_RELEASE);
	TachyonAssertMsg(rc == true, "Failed to free JIT code memory");
#else
	TachyonAssertMsg(munmap(Base, Size) != -1, "Failed to free JIT code memory");
#endif
}

bool Tachyon::ProtectCodeMemory(void * Base, const size_t Size) {
	if (Base == nullptr) {
		return false;
	}
#if (defined(_WIN32) || defined(_WIN64))
	DWORD PreviousProtection;
	bool rc = VirtualProtect(Base, Size, PAGE_EXECUTE_READ, &PreviousProtection);
	return rc;
#else
	return mprotect(Base, Size, (PROT_EXEC | PROT_READ)) != -1 ? true : false;
#endif
}