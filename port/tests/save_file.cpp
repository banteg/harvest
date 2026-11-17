// zig build test-save_file. Uses a new directory in the test's working directory.
#include "game/SaveFile.h"
#include "harvest/settings/CSavestateInfo.h"
#include "ox/io/CMemReadFile.h"
#include "ox/io/CMemWriteFile.h"
#include <SDL3/SDL.h>
#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#if !defined(__EMSCRIPTEN__)
#include <signal.h>
#include <sys/resource.h>
#endif
#endif

using harvest::settings::CSavestateInfo;
using harvest::settings::SSavestateHeader;
using ox::io::CMemReadFile;
using ox::io::CMemWriteFile;
typedef std::vector<unsigned char> Bytes;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "Save file check failed: %s\n", #c); return false; } } while (0)

static bool readSlot(const char* path, Bytes& bytes)
{
    size_t size = 0;
    unsigned char* data = (unsigned char*)SDL_LoadFile(path, &size);
    CHECK(data);
    bytes.assign(data, data + size);
    SDL_free(data);
    return true;
}

static bool roundTrip(const char* path, const SSavestateHeader& header, const Bytes& plain,
                      bool expands, Bytes& encodedPayload)
{
    CMemWriteFile payload;
    payload.write(&plain[0], (int)plain.size());
    CHECK(port::writeSaveFile(path, header, payload));
    Bytes slot;
    CHECK(readSlot(path, slot) && !slot.empty());
    CMemReadFile input(&slot[0], (int)slot.size(), false);
    SSavestateHeader loaded;
    CHECK(CSavestateInfo::readHeader(&input, loaded));
    CHECK(loaded.Magic == 0x123123ff && loaded.Version == 30);
    CHECK(loaded.PlayerName == header.PlayerName && loaded.Description == header.Description);
    CHECK(loaded.GameMode == header.GameMode && loaded.ThreatLevel == header.ThreatLevel &&
          loaded.Unknown == header.Unknown && loaded.Planet == header.Planet && loaded.Time == header.Time);
    encodedPayload.assign(slot.begin() + input.getPos(), slot.end());
    int sizes[3];
    CHECK(encodedPayload.size() >= sizeof(sizes));
    std::memcpy(sizes, &encodedPayload[0], sizeof(sizes));
    CHECK(sizes[0] > 0 && sizes[1] == (int)plain.size() && (sizes[0] > sizes[1]) == expands);
    CHECK(sizes[2] == ((sizes[0] + 15) & ~15) && encodedPayload.size() == sizeof(sizes) + sizes[2]);

    CMemReadFile* decoded = port::readSavePayload(&input);
    CHECK(decoded);
    const bool identical = decoded->getSize() == (int)plain.size() &&
        std::memcmp(decoded->getCurrentPointer(), &plain[0], plain.size()) == 0;
    decoded->drop();
    CHECK(identical && input.getRemainingSize() == 0);
    return true;
}

static bool rejected(Bytes bytes, size_t length)
{
    CMemReadFile input(&bytes[0], (int)length, false);
    CMemReadFile* decoded = port::readSavePayload(&input);
    if (decoded)
        decoded->drop();
    return decoded == 0;
}

static bool testLengths(const Bytes& valid)
{
    CHECK(rejected(valid, 0) && rejected(valid, 3 * sizeof(int) - 1));
    CHECK(rejected(valid, valid.size() - 1));
    int sizes[3];
    std::memcpy(sizes, &valid[0], sizeof(sizes));
    // The compressed one-byte sample fits inside a padded AES block. Adjusting its length by
    // one leaves the AES size valid, so these cases require inspecting the actual zlib stream.
    CHECK(sizes[0] > 1 && sizes[0] + 1 < sizes[2]);
    const int changes[][2] = {
        {0, 0}, {0, -1}, {0, INT_MAX}, {0, sizes[0] - 1}, {0, sizes[0] + 1},
        {1, 0}, {1, -1}, {1, sizes[1] + 1}, {1, INT_MAX},
        {2, 0}, {2, -1}, {2, sizes[2] - 1}, {2, sizes[2] + 16}
    };
    for (size_t i = 0; i < sizeof(changes) / sizeof(changes[0]); ++i)
    {
        Bytes altered = valid;
        std::memcpy(&altered[changes[i][0] * sizeof(int)], &changes[i][1], sizeof(int));
        CHECK(rejected(altered, altered.size()));
    }
    return true;
}

static bool testSlot(const char* path)
{
    // One 0xff byte, compressed with zlib and encrypted independently with OpenSSL using the
    // original save key. This also catches matching encoder/decoder changes that break old slots.
    unsigned char fixture[] = {
        9,0,0,0, 1,0,0,0, 16,0,0,0,
        0x99,0xee,0x0c,0x61,0x3e,0x14,0xe5,0xe6,0x91,0x45,0x98,0x65,0x33,0xb6,0xa4,0x2a
    };
    CMemReadFile reference(fixture, sizeof(fixture), false);
    CMemReadFile* decoded = port::readSavePayload(&reference);
    CHECK(decoded);
    unsigned char byte;
    const bool originalFormat = decoded->getSize() == 1 && decoded->read(&byte, 1) == 1 && byte == 0xff;
    decoded->drop();
    CHECK(originalFormat);

    SSavestateHeader header = {};
    header.PlayerName = L"Player";
    header.Description = L"Slot replacement";
    header.GameMode = 4;
    header.ThreatLevel = 7;
    header.Unknown = 1234;
    header.Planet = 2;
    header.Time = 123456789;
    Bytes encoded;
    CHECK(roundTrip(path, header, Bytes(1, 0xff), true, encoded));
    CHECK(testLengths(encoded));
    CHECK(roundTrip(path, header, Bytes(8192, 0), false, encoded));

    Bytes noise(128 * 1024);
    Uint32 state = 1;
    for (size_t i = 0; i < noise.size(); ++i)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        noise[i] = (unsigned char)state;
    }
    CHECK(roundTrip(path, header, noise, true, encoded));

#if defined(RLIMIT_FSIZE) && defined(SIGXFSZ)
    Bytes original;
    CHECK(readSlot(path, original));
    CMemWriteFile replacement;
    noise[0] ^= 0xff;
    replacement.write(&noise[0], (int)noise.size());
    struct rlimit prior;
    CHECK(getrlimit(RLIMIT_FSIZE, &prior) == 0);
    struct rlimit limited = prior;
    limited.rlim_cur = 4096;
    struct sigaction ignore = {}, previous;
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    CHECK(sigaction(SIGXFSZ, &ignore, &previous) == 0);
    CHECK(setrlimit(RLIMIT_FSIZE, &limited) == 0);
    const bool saved = port::writeSaveFile(path, header, replacement);
    CHECK(setrlimit(RLIMIT_FSIZE, &prior) == 0);
    CHECK(sigaction(SIGXFSZ, &previous, 0) == 0);
    Bytes afterFailure;
    CHECK(!saved && readSlot(path, afterFailure) && afterFailure == original);
#endif
    return true;
}

int main()
{
    char directory[80];
    SDL_snprintf(directory, sizeof(directory), "harvest-save-test-%llx-%x",
        (unsigned long long)SDL_GetPerformanceCounter(), (unsigned)SDL_rand_bits());
#if defined(_WIN32)
    const int created = _mkdir(directory);
#else
    const int created = mkdir(directory, 0700);
#endif
    if (created != 0)
        return 1;
    const std::string path = std::string(directory) + "/slot.hsg";
    bool passed = testSlot(path.c_str());
    SDL_RemovePath(path.c_str());
    // A failed save must also remove its partially written staging file.
    if (!SDL_RemovePath(directory))
    {
        std::fprintf(stderr, "Save directory cleanup failed: %s\n", SDL_GetError());
        passed = false;
    }
    return passed ? 0 : 1;
}
