// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CBase64url.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace algo {

//! The URL-safe base64 alphabet, then the padding character.
static const char Base64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=";

//! The value of each character from '-' to 'z' in Base64Alphabet, 0xff for the others; the
//! padding character decodes as 0.
static const unsigned char Base64Inv[] =
{
    0x3e, 0xff, 0xff, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff,
    0x00, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff,
    0xff, 0xff, 0x3f, 0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
};

//! The standard base64 alphabet and padding, percent-encoded for URLs.
static const char* Base64Alphabet2[] =
{
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
    "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "a", "b", "c", "d", "e", "f",
    "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v",
    "w", "x", "y", "z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "%2B", "%2F",
    "%3D"
};

//! Characters from '-' up to 'z' have a Base64Inv entry.
static const char FIRST_CHARACTER = '-';
//! Base64Inv values above this mark invalid characters; the bound is the table's last index.
static const unsigned char LAST_VALUE = 77;

void CBase64url::encode(core::CString<char>& result, io::IReadFile* file)
{
    result = "";
    char in[3];
    int read;
    while ((read = file->read(in, 3)) == 3)
    {
        result.append(Base64Alphabet[(in[0] & 0xfc) >> 2]);
        result.append(Base64Alphabet[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)]);
        result.append(Base64Alphabet[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)]);
        result.append(Base64Alphabet[in[2] & 0x3f]);
    }

    switch (read)
    {
    case 1:
        in[1] = 0;
        result.append(Base64Alphabet[(in[0] & 0xfc) >> 2]);
        result.append(Base64Alphabet[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)]);
        result.append(core::CString<char>("=="));
        break;
    case 2:
        in[2] = 0;
        result.append(Base64Alphabet[(in[0] & 0xfc) >> 2]);
        result.append(Base64Alphabet[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)]);
        result.append(Base64Alphabet[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)]);
        result.append(core::CString<char>("="));
        break;
    }
}

void CBase64url::encode2(core::CString<char>& result, io::IReadFile* file)
{
    result = "";
    char in[3];
    int read;
    while ((read = file->read(in, 3)) == 3)
    {
        result.append(core::CString<char>(Base64Alphabet2[(in[0] & 0xfc) >> 2]));
        result.append(core::CString<char>(Base64Alphabet2[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)]));
        result.append(core::CString<char>(Base64Alphabet2[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)]));
        result.append(core::CString<char>(Base64Alphabet2[in[2] & 0x3f]));
    }

    switch (read)
    {
    case 1:
        in[1] = 0;
        result.append(core::CString<char>(Base64Alphabet2[(in[0] & 0xfc) >> 2]));
        result.append(core::CString<char>(Base64Alphabet2[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)]));
        result.append(core::CString<char>("%3D%3D"));
        break;
    case 2:
        in[2] = 0;
        result.append(core::CString<char>(Base64Alphabet2[(in[0] & 0xfc) >> 2]));
        result.append(core::CString<char>(Base64Alphabet2[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)]));
        result.append(core::CString<char>(Base64Alphabet2[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)]));
        result.append(core::CString<char>("%3D"));
        break;
    }
}

bool CBase64url::decode(io::IWriteFile* file, const core::CString<char>& text)
{
    if (text.size() % 4 != 0)
        return false;

    for (int i = 0; i < text.size(); i += 4)
    {
        unsigned char a = Base64Inv[text[i] - FIRST_CHARACTER];
        unsigned char b = Base64Inv[text[i + 1] - FIRST_CHARACTER];
        if (a > LAST_VALUE || b > LAST_VALUE)
            return false;
        unsigned char byte = (a << 2) | (b >> 4);
        file->write(&byte, 1);

        unsigned char c = Base64Inv[text[i + 2] - FIRST_CHARACTER];
        if (c > LAST_VALUE)
            return false;
        byte = (b << 4) | (c >> 2);
        file->write(&byte, 1);

        unsigned char d = Base64Inv[text[i + 3] - FIRST_CHARACTER];
        if (d > LAST_VALUE)
            return false;
        byte = (c << 6) | d;
        file->write(&byte, 1);
    }
    return true;
}

} // end namespace algo
} // end namespace ox
