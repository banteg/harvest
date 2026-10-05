// Recovered for Harvest; not the original source.
#ifndef OX_ALGO_CBASE64URL_H
#define OX_ALGO_CBASE64URL_H
#include "ox/core/CString.h"
#include "ox/io/IReadFile.h"
#include "ox/io/IWriteFile.h"
namespace ox { namespace algo {
class CBase64url
{
public:
    static void encode(core::CString<char>& result, io::IReadFile* file);
    //! Writes the bytes encoded in text to file.
    static void decode(io::IWriteFile* file, const core::CString<char>& text);
};
} }
#endif
