#include <Tachyon/Debug.hpp>
#include <Tachyon/ExMem.hpp>

#include <cstdint>
#include <dlfcn.h>
#include <unistd.h>

#include <charconv>
#include <fstream>
#include <string>

/*
 * this should only be used for linux systems
 */

static void * JITRegionStart = nullptr;

void * Tachyon::GetJITRegionStart(void) {
    return JITRegionStart;
}

bool Tachyon::ReadProcMaps(void) {
    Dl_info Info;
    const int rc = dladdr(reinterpret_cast<void *>(ReadProcMaps), &Info);
    TachyonAssertMsg(rc != 0, "Failed to get program base address.\n");

    const uint64_t ProgramBase = std::bit_cast<uint64_t>(Info.dli_fbase);
    const size_t PageSize = sysconf(_SC_PAGESIZE);

    std::ifstream ProcMapsFile("/proc/self/maps");
    TachyonAssertMsg(ProcMapsFile.is_open(), "Cannot open /proc/self/maps.");

    std::string Line;

    uint64_t LastEndAddress = 0;

    while(std::getline(ProcMapsFile, Line)) {
        size_t DashPos = Line.find('-');
        size_t SpacePos = Line.find(' ');

        if (DashPos == std::string::npos || SpacePos == std::string::npos) {
            continue;
        }

        uint64_t StartAddress;
        uint64_t EndAddress;

        auto ConvResult = std::from_chars(Line.data(), Line.data() + DashPos, StartAddress, 16);
        TachyonAssert(ConvResult.ec == std::errc {});
        ConvResult = std::from_chars(Line.data() + (DashPos + 1), Line.data() + SpacePos, EndAddress, 16);
        TachyonAssert(ConvResult.ec == std::errc {});

        if (LastEndAddress != 0) {
            /* just in case it isn't aligned already */
            const uint64_t GapStart = (LastEndAddress + PageSize - 1) & ~(PageSize - 1);
            const uint64_t GapEnd = StartAddress;

            const size_t RegionSize = GapEnd - GapStart;
            /* is it in a reachable range? */
            if (GapStart < ProgramBase + INT32_MAX || GapStart > ProgramBase + INT32_MIN) {
                /* is it big enough? */
                if (RegionSize >= INT32_MAX) {
                    /* holy moly */
                    DebugInfo("Found potential JIT code buffer range: 0x%016zx - 0x%016zx\n", GapStart, GapEnd);

                    JITRegionStart = reinterpret_cast<void *>(GapStart);

                    ProcMapsFile.close();
                    return true;
                }
            }
        }
        LastEndAddress = EndAddress;
    }
    ProcMapsFile.close();
    return false;
}
