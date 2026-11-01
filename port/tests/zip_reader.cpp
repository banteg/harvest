#include "daisy/io/CZipReader.h"
#include "ox/io/CMemReadFile.h"
#include <cstdio>
#include <string>
#include <vector>

typedef std::vector<unsigned char> Bytes;

static Bytes archive(const std::string& name)
{
    Bytes bytes(30, 0);
    bytes[0] = 'P'; bytes[1] = 'K'; bytes[2] = 3; bytes[3] = 4;
    bytes[18] = bytes[22] = 1; // One stored byte.
    bytes[26] = (unsigned char)name.size();
    bytes[27] = (unsigned char)(name.size() >> 8);
    bytes.insert(bytes.end(), name.begin(), name.end());
    bytes.push_back('x');
    return bytes;
}

static bool check(Bytes bytes, const std::string& name, bool valid)
{
    ox::io::CMemReadFile file(&bytes[0], bytes.size(), false);
    daisy::io::CZipReader zip(&file, false, false);
    if (!valid)
        return zip.getFileCount() == 0;
    if (zip.getFileCount() != 1)
        return false;
    ox::io::IReadFile* stored = zip.openFile(name.c_str());
    if (!stored)
        return false;
    char byte = 0;
    bool ok = stored->read(&byte, 1) == 1 && byte == 'x';
    stored->drop();
    return ok;
}

int main()
{
    const size_t lengths[] = {8, 1023, 1024, 32768, 65535};
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i)
    {
        std::string name = "dir/" + std::string(lengths[i] - 4, 'a');
        if (!check(archive(name), name, true))
            return 1;
    }
    Bytes truncated = archive("short");
    truncated[26] = 255;
    if (!check(truncated, "", false) || !check(archive(std::string("a\0b", 3)), "", false))
        return 1;
    std::puts("ZIP stored entries and filename bounds passed");
    return 0;
}
