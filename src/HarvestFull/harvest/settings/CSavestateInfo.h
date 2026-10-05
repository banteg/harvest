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

//! The header of a saved game, shown in the save game list.
struct SSavestateHeader
{
    //! 0x123123ff in a valid file.
    int Magic;
    int Version;
    ox::core::CString<wchar_t> PlayerName;
    int GameMode;
    int ThreatLevel;
    int Minerals;
    int Planet;
    //! When the game was saved, as returned by time().
    int Time;
    ox::core::CString<wchar_t> Description;
};

//! Reads and writes saved game headers.
class CSavestateInfo
{
public:
    static bool readHeader(ox::io::IReadFile* file, SSavestateHeader& header);
    static bool writeHeader(ox::io::IWriteFile* file, const SSavestateHeader& header);
};

} // end namespace settings
} // end namespace harvest

#endif
