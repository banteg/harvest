// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef OX_GAME_CCONFIGURATION_H
#define OX_GAME_CCONFIGURATION_H

#include "../TArray.h"
#include "../core/CString.h"

namespace ox {
namespace io { class IFileSystem; class IReadFile; }
namespace game {

//! A configuration file of blocks and attributes, such as "mod:name".
class CConfiguration
{
public:
    CConfiguration(io::IFileSystem* fileSystem);
    virtual ~CConfiguration();

    bool read(io::IReadFile* file);
    bool attributeExists(const wchar_t* name);
    bool getAttribute(const wchar_t* name, core::CString<wchar_t>& value);

private:
    io::IFileSystem* FileSystem;
    // The parsed blocks; their type is not recovered yet.
    TArray<void*> Blocks;
};

} // end namespace game
} // end namespace ox

#endif
