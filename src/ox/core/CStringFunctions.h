// Recovered for Harvest; not the original source. Partial: only what recovered units use.

#ifndef OX_CORE_CSTRINGFUNCTIONS_H
#define OX_CORE_CSTRINGFUNCTIONS_H

#include <cstdlib>
#include "CString.h"
#include "../TArray.h"

namespace ox {
namespace core {

class CStringFunctions
{
public:
    //! Converts a wide string to the current locale's multibyte encoding.
    static CString<char> wideToAnsi(const CString<wchar_t>& str)
    {
        int size = str.size() + 1;
        char* ansi = new char[size];
        const wchar_t* src = str.c_str();
        wchar_t* wide = new wchar_t[size];
        for (int i = 0; i < size; ++i)
            wide[i] = src[i];
        wcstombs(ansi, wide, size);
        // scalar delete of an array, as in the Linux build
        delete wide;
        ansi[size - 1] = 0;
        CString<char> result = ansi;
        delete [] ansi;
        // an explicit copy: both builds copy result instead of constructing it in place
        return CString<char>(result);
    }

    static CString<wchar_t> ansiToWide(const CString<char>& str);
    //! Formats a time given in seconds as [h:]mm:ss.hh; hours are shown from one hour on with showHours.
    static CString<wchar_t> millisecondsToWide(float milliseconds, bool showHours);
    // Defined inline in CStringConversions.h.
    static float wideToFloat(const wchar_t* str);
    static CString<wchar_t> floatToWide(float value, char* format = 0);
};

//! A copy of source with every occurrence of find replaced by replacement.
template <class T>
CString<T> replaceAll(const CString<T>& source, const CString<T>& find, const CString<T>& replacement)
{
    CString<T> result;
    int position = 0;
    int found = source.findNext(find.c_str(), 0);
    while (found != -1)
    {
        result.append(source.subString(position, found - position));
        result.append(replacement);
        position = found + find.size();
        int next = source.subStringToEnd(position).findNext(find.c_str(), 0);
        found = next + position;
        if (next == -1)
            found = -1;
    }
    result.append(source.subStringToEnd(position));
    return result;
}

//! Splits str at every occurrence of separator into parts.
template <class T>
void splitString(TArray<CString<T> >& parts, const CString<T>& str, const CString<T>& separator)
{
    CString<T> rest = str;
    int separatorLength = separator.size();
    do
    {
        int position = rest.findNext(separator.c_str(), 0);
        if (position < 0)
        {
            parts.push_back(rest);
            break;
        }
        parts.push_back(rest.subString(0, position));
        rest = rest.subStringToEnd(position + separatorLength);
    } while (rest.size() > 0);
}

} // end namespace core
} // end namespace ox

#endif
