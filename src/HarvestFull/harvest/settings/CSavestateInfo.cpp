// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "CSavestateInfo.h"
#include "ox/io/CHelpIO.h"

namespace harvest {
namespace settings {

//! Marks a save game file.
static const int SAVESTATE_MAGIC = 0x123123ff;
//! The save game version this build writes; it reads versions OLDEST_SAVESTATE_VERSION and up.
static const int SAVESTATE_VERSION = 30;
static const int OLDEST_SAVESTATE_VERSION = 26;
//! The first version with the planet, time and description fields.
static const int SAVESTATE_DESCRIPTION_VERSION = 18;

bool CSavestateInfo::readHeader(ox::io::IReadFile* file, SSavestateHeader& header)
{
    bool result = false;
    if (file)
    {
        header.Magic = ox::io::CHelpIO::readInt(file);
        header.Version = ox::io::CHelpIO::readInt(file);
        if (header.Magic == SAVESTATE_MAGIC && header.Version <= SAVESTATE_VERSION &&
            header.Version >= OLDEST_SAVESTATE_VERSION)
        {
            ox::io::CHelpIO::readWideString(file, header.PlayerName);
            header.GameMode = ox::io::CHelpIO::readInt(file);
            header.ThreatLevel = ox::io::CHelpIO::readInt(file);
            header.Unknown = ox::io::CHelpIO::readInt(file);
            if (header.Version < SAVESTATE_DESCRIPTION_VERSION)
            {
                header.Planet = 0;
                header.Time = 0;
            }
            else
            {
                header.Planet = ox::io::CHelpIO::readInt(file);
                header.Time = ox::io::CHelpIO::readInt(file);
                ox::io::CHelpIO::readWideString(file, header.Description);
            }
            result = true;
        }
    }
    return result;
}

bool CSavestateInfo::writeHeader(ox::io::IWriteFile* file, const SSavestateHeader& header)
{
    bool result = false;
    if (file)
    {
        ox::io::CHelpIO::writeInt(file, SAVESTATE_MAGIC);
        ox::io::CHelpIO::writeInt(file, SAVESTATE_VERSION);
        ox::io::CHelpIO::writeWideString(file, header.PlayerName, true);
        ox::io::CHelpIO::writeInt(file, header.GameMode);
        ox::io::CHelpIO::writeInt(file, header.ThreatLevel);
        ox::io::CHelpIO::writeInt(file, header.Unknown);
        ox::io::CHelpIO::writeInt(file, header.Planet);
        ox::io::CHelpIO::writeInt(file, header.Time);
        ox::io::CHelpIO::writeWideString(file, header.Description, true);
        result = true;
    }
    return result;
}

} // end namespace settings
} // end namespace harvest
