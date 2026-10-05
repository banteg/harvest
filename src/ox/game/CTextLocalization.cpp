// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CTextLocalization.h"
#include "ox/core/CStringFunctions.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace game {

core::CString<wchar_t> CTextLocalization::getText(const wchar_t* key)
{
    core::CString<wchar_t> text;
    getAttribute(key, text);
    text = core::replaceAll(text, core::CString<wchar_t>(L"\\n"), core::CString<wchar_t>(L"\n"));
    return core::CString<wchar_t>(text);
}

} // end namespace game
} // end namespace ox
