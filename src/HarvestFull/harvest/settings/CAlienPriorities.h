// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_SETTINGS_CALIENPRIORITIES_H
#define HARVEST_SETTINGS_CALIENPRIORITIES_H

#include "ox/core/CString.h"

namespace ox {
namespace io {
class IReadFile;
class IWriteFile;
}
}

namespace harvest {
namespace settings {

//! Weapon targeting preferences, indexed by alien type.
class CAlienPriorities
{
public:
    CAlienPriorities();
    virtual ~CAlienPriorities();

    bool read(ox::io::IReadFile* file, int version);
    bool write(ox::io::IWriteFile* file);
    void setPrioritiesFromString(const ox::core::CString<wchar_t>& text);
    ox::core::CString<wchar_t> prioritiesToString() const;
    void setAlienPriority(int alienType, int priority);
    void setIfRangeIsImportant(bool important);
    void setHoldFire(bool hold);
    int getAlienPriority(int alienType) const { return Priorities[alienType]; }
    bool getIfRangeIsImportant() const { return RangeIsImportant; }
    bool getHoldFire() const { return HoldFire; }

    int Priorities[14];
    bool RangeIsImportant;
    bool HoldFire;
};

//! Five independently configurable weapon priority sets.
extern CAlienPriorities g_attackPriorities[5];

} // end namespace settings
} // end namespace harvest

#endif
