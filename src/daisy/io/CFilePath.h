// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Defined inline: the Linux build inlines the constructor into CFileSystem::resolveAliases and
// keeps the rest as COMDAT copies in that object.

#ifndef DAISY_IO_CFILEPATH_H
#define DAISY_IO_CFILEPATH_H

#include "ox/io/IFilePath.h"
#include "ox/core/CString.h"

namespace daisy {
namespace io {

//! A path with its directory aliases resolved, from CFileSystem::resolveAliases.
class CFilePath : public ox::io::IFilePath
{
public:
    CFilePath(const char* path)
        : Path(path)
    {
    }

    virtual ~CFilePath() {}

    virtual const char* getPath() { return Path.c_str(); }

private:
    ox::core::CString<char> Path;
};

} // end namespace io
} // end namespace daisy

#endif
