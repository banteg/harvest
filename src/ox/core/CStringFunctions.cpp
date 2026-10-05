// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CStringFunctions.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace core {

CString<wchar_t> CStringFunctions::millisecondsToWide(float seconds, bool showHours)
{
    CString<wchar_t> result;
    int milliseconds = (int)(seconds * 1000);

    if (milliseconds >= 3600000 && showHours)
    {
        int hours = milliseconds / 3600000;
        milliseconds -= hours * 3600000;
        result.append(hours);
        result.append(CString<wchar_t>(L":"));
    }

    int minutes = milliseconds / 60000;
    if (minutes < 10)
        result.append(CString<wchar_t>(L"0"));
    result.append(minutes);
    milliseconds -= minutes * 60000;

    int wholeSeconds = milliseconds / 1000;
    if (wholeSeconds < 10)
        result.append(CString<wchar_t>(L":0"));
    else
        result.append(CString<wchar_t>(L":"));
    result.append(wholeSeconds);
    milliseconds -= wholeSeconds * 1000;

    int hundredths = milliseconds / 10;
    if (hundredths < 10)
        result.append(CString<wchar_t>(L".0"));
    else
        result.append(CString<wchar_t>(L"."));
    result.append(hundredths);

    return CString<wchar_t>(result);
}

} // end namespace core
} // end namespace ox
