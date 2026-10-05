// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Inline bodies of CStringFunctions conversions that the configuration units inline. They live
// apart from CStringFunctions.h because more inline functions there change the code GCC
// generates for other units that include it.

#ifndef OX_CORE_CSTRINGCONVERSIONS_H
#define OX_CORE_CSTRINGCONVERSIONS_H

#include <cstdio>
#include <cstdlib>
#include "CStringFunctions.h"

namespace ox {
namespace core {

//! Converts a string in the current locale's multibyte encoding to a wide string.
inline CString<wchar_t> CStringFunctions::ansiToWide(const CString<char>& str)
{
    int size = str.size() + 1;
    wchar_t* wide = new wchar_t[size];
    const char* src = str.c_str();
    wchar_t* converted = new wchar_t[size];
    mbstowcs(converted, src, size);
    for (int i = 0; i < size; ++i)
        wide[i] = converted[i];
    // scalar delete of an array, as in both builds
    delete converted;
    wide[size - 1] = 0;
    CString<wchar_t> result = wide;
    delete [] wide;
    return CString<wchar_t>(result);
}

//! Parses a decimal number; characters are narrowed one by one.
inline float CStringFunctions::wideToFloat(const wchar_t* str)
{
    CString<char> ansi = str;
    return (float)atof(ansi.c_str());
}

//! Formats a number with printf's format, "%.2f" by default.
inline CString<wchar_t> CStringFunctions::floatToWide(float value, char* format)
{
    char buffer[20];
    if (format)
        sprintf(buffer, format, value);
    else
        sprintf(buffer, "%.2f", value);
    return CString<wchar_t>(buffer);
}

} // end namespace core
} // end namespace ox

#endif
