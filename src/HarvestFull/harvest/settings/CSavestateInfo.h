// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CSAVESTATEINFO_H
#define HARVEST_SETTINGS_CSAVESTATEINFO_H

#include "ox/core/CString.h"

namespace ox {
namespace io {
class IReadFile;
class IWriteFile;
} // end namespace io
} // end namespace ox

namespace harvest {
namespace settings {

//! The header at the start of a save game file.
struct SSavestateHeader
{
    //! 0x123123ff.
    int Magic;
    int Version;
    ox::core::CString<wchar_t> PlayerName;
    //! As in harvest::game::EGAME_MODE.
    int GameMode;
    int ThreatLevel;
    // Not read by recovered code.
    int Unknown;
    int Planet;
    //! Seconds since the epoch.
    int Time;
    ox::core::CString<wchar_t> Description;
};

//! Reads and writes save game headers.
class CSavestateInfo
{
public:
    static bool readHeader(ox::io::IReadFile* file, SSavestateHeader& header);
    static bool writeHeader(ox::io::IWriteFile* file, const SSavestateHeader& header);
};

} // end namespace settings
} // end namespace harvest

#endif
