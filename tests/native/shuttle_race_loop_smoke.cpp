#include <ox/core/CBasic.h>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include "source-loop.h"
#include "target-loop.h"

typedef int (*Predicate)(float);

static bool check(Predicate target, unsigned int bits)
{
    float delta;
    std::memcpy(&delta, &bits, sizeof(delta));
    // Check the loop boundary directly and through updateState's real clamp helper.
    float values[] = {delta, ox::core::clamp(delta, 0.0f, 0.06f)};
    for (int i = 0; i < 2; ++i)
        if (sourceEnters(values[i]) != (target(values[i]) != 0))
        {
            std::fprintf(stderr, "Shuttle-race loop check failed: bits=%08x clamped=%d\n", bits, i);
            return false;
        }
    return true;
}

int main()
{
    void* memory = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED)
        return 2;
    std::memcpy(memory, targetCode, sizeof(targetCode));
    if (mprotect(memory, 4096, PROT_READ | PROT_EXEC) != 0)
        return 2;
    Predicate target = reinterpret_cast<Predicate>(memory);
    // Signed zero, subnormals, finite values, infinities, quiet and signaling NaNs.
    unsigned int edges[] = {0, 0x80000000, 1, 0x80000001, 0x3c23d70a, 0xbc23d70a,
        0x7f7fffff, 0xff7fffff, 0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000,
        0x7f800001, 0xff800001};
    for (unsigned int i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
        if (!check(target, edges[i]))
            return 1;
    unsigned int bits = 3;
    for (int i = 0; i < 5000; ++i)
    {
        bits = bits * 1664525u + 1013904223u;
        if (!check(target, bits))
            return 1;
    }
    munmap(memory, 4096);
    std::puts("Shuttle-race loop smoke passed: original binary predicate, edge cases and 5000 bit patterns");
    return 0;
}
