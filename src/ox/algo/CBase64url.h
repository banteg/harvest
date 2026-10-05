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
    //! Sets result to the file's remaining data as URL-safe base64 with padding.
    static void encode(core::CString<char>& result, io::IReadFile* file);
    //! Like encode, but with the standard alphabet percent-encoded for URLs.
    static void encode2(core::CString<char>& result, io::IReadFile* file);
    //! Writes the data that text encodes to file.
    static bool decode(io::IWriteFile* file, const core::CString<char>& text);
};
} }
#endif
