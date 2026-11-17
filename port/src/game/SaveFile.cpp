#include "game/SaveFile.h"
#include "harvest/settings/CSavestateInfo.h"
#include "ox/algo/CRand.h"
#include "ox/core/CAes.h"
#include "ox/io/CHelpIO.h"
#include "ox/io/CMemReadFile.h"
#include "ox/io/CMemWriteFile.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_timer.h>
#include <zlib.h>
#include <new>
#include <string>
#include <vector>

namespace port {
namespace {

// Bound both directions, including the allocation requested by the plaintext size field.
const int MAX_PAYLOAD = 128 * 1024 * 1024;

bool cryptSave(unsigned char* dst, const unsigned char* src, int size, bool encrypt)
{
    ox::algo::CRand random(1);
    unsigned char bytes[32];
    for (int i = 0; i < 32; ++i)
        bytes[i] = random.nextInt(256);
    ox::core::CCipherKey key(bytes, sizeof(bytes));
    ox::core::CAes aes;
    aes.setKey(&key);
    return encrypt ? aes.encrypt(dst, src, size) : aes.decrypt(dst, src, size);
}

}

ox::io::CMemReadFile* readSavePayload(ox::io::IReadFile* file)
{
    // The original and all supported port targets store these integers little-endian.
    int sizes[3];
    if (file->read(sizes, sizeof(sizes)) != sizeof(sizes))
        return 0;
    const int compressedSize = sizes[0], size = sizes[1], encryptedSize = sizes[2];
    const int pos = file->getPos();
    if (size <= 0 || size > MAX_PAYLOAD || compressedSize <= 0 ||
        (unsigned)compressedSize > compressBound(MAX_PAYLOAD) ||
        encryptedSize != ((compressedSize + 15) & ~15) ||
        pos < 0 || pos > file->getSize() || encryptedSize > file->getSize() - pos)
        return 0;

    std::vector<unsigned char> encrypted(encryptedSize);
    if (file->read(&encrypted[0], encryptedSize) != encryptedSize)
        return 0;
    std::vector<unsigned char> compressed(encryptedSize);
    if (!cryptSave(&compressed[0], &encrypted[0], encryptedSize, false))
        return 0;

    char* unpacked = new (std::nothrow) char[size];
    if (!unpacked)
        return 0;
    uLongf written = size;
    uLong consumed = compressedSize;
    if (uncompress2((Bytef*)unpacked, &written, &compressed[0], &consumed) != Z_OK ||
        written != (unsigned)size || consumed != (unsigned)compressedSize)
    {
        delete[] unpacked;
        return 0;
    }
    return new ox::io::CMemReadFile(unpacked, size, true);
}

bool writeSaveFile(const char* path, const harvest::settings::SSavestateHeader& header,
    ox::io::CMemWriteFile& payload)
{
    const int size = payload.getSize();
    if (size <= 0 || size > MAX_PAYLOAD)
        return false;
    uLongf compressedSize = compressBound(size);
    std::vector<unsigned char> compressed(compressedSize);
    if (compress(&compressed[0], &compressedSize, (const Bytef*)payload.getData(), size) != Z_OK)
        return false;
    const int encryptedSize = ((int)compressedSize + 15) & ~15;
    std::vector<unsigned char> encrypted(encryptedSize);
    if (!cryptSave(&encrypted[0], &compressed[0], (int)compressedSize, true))
        return false;
    ox::io::CMemWriteFile prefix;
    harvest::settings::CSavestateInfo::writeHeader(&prefix, header);
    ox::io::CHelpIO::writeInt(&prefix, (int)compressedSize);
    ox::io::CHelpIO::writeInt(&prefix, size);
    ox::io::CHelpIO::writeInt(&prefix, encryptedSize);

    // Create exclusively, in the slot's directory so replacing it is a single rename.
    char suffix[48];
    SDL_snprintf(suffix, sizeof(suffix), ".%llx-%x.tmp",
        (unsigned long long)SDL_GetPerformanceCounter(), (unsigned)SDL_rand_bits());
    const std::string temporary = std::string(path) + suffix;
    SDL_IOStream* out = SDL_IOFromFile(temporary.c_str(), "wbx");
    if (!out)
        return false;
    bool saved = SDL_WriteIO(out, prefix.getData(), prefix.getSize()) == (size_t)prefix.getSize() &&
        SDL_WriteIO(out, &encrypted[0], encryptedSize) == (size_t)encryptedSize && SDL_FlushIO(out);
    if (!SDL_CloseIO(out))
        saved = false;
    if (saved && SDL_RenamePath(temporary.c_str(), path))
        return true;
    SDL_RemovePath(temporary.c_str());
    return false;
}

}
