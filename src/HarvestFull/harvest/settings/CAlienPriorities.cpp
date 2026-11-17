// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "harvest/settings/CAlienPriorities.h"
#include "ox/io/CHelpIO.h"
#include <wchar.h>

namespace harvest {
namespace settings {

CAlienPriorities g_attackPriorities[5];

CAlienPriorities::CAlienPriorities() : RangeIsImportant(false), HoldFire(false)
{
    for (int i = 0; i < 14; ++i)
        Priorities[i] = 2;
}

CAlienPriorities::~CAlienPriorities()
{
}

bool CAlienPriorities::read(ox::io::IReadFile* file, int version)
{
    for (int i = 0; i < 14; ++i)
        Priorities[i] = ox::io::CHelpIO::readInt(file);
    RangeIsImportant = ox::io::CHelpIO::readInt(file) != 0;
    if (version >= 30)
        HoldFire = ox::io::CHelpIO::readInt(file) != 0;
    return true;
}

bool CAlienPriorities::write(ox::io::IWriteFile* file)
{
    for (int i = 0; i < 14; ++i)
        ox::io::CHelpIO::writeInt(file, Priorities[i]);
    ox::io::CHelpIO::writeInt(file, RangeIsImportant);
    ox::io::CHelpIO::writeInt(file, HoldFire);
    return true;
}

void CAlienPriorities::setPrioritiesFromString(const ox::core::CString<wchar_t>& text)
{
    int comma = text.findFirst(L',');
    int index = 0;
    int start = 0;
    while (comma != -1 && index < 14)
    {
        ox::core::CString<wchar_t> value = text.subString(start, comma - start);
        Priorities[index] = wcstol(value.c_str(), 0, 10);
        start = comma + 1;
        ++index;
        comma = text.findNext(L',', start);
    }
    // The original includes the terminating character and accepts an empty final token.
    if (start <= text.size() && index < 14)
    {
        ox::core::CString<wchar_t> value = text.subString(start, text.size() + 1 - start);
        Priorities[index] = wcstol(value.c_str(), 0, 10);
    }
}

ox::core::CString<wchar_t> CAlienPriorities::prioritiesToString() const
{
    ox::core::CString<wchar_t> result;
    for (int i = 0; i < 14; ++i)
    {
        if (i > 0)
            result.append(ox::core::CString<wchar_t>(L","));
        result.append(Priorities[i]);
    }
    return ox::core::CString<wchar_t>(result);
}

void CAlienPriorities::setAlienPriority(int alienType, int priority)
{
    Priorities[alienType] = priority;
}

void CAlienPriorities::setIfRangeIsImportant(bool important)
{
    RangeIsImportant = important;
}

void CAlienPriorities::setHoldFire(bool hold)
{
    HoldFire = hold;
}

} // end namespace settings
} // end namespace harvest
