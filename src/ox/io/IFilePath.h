// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The file name is inferred; the Mac vtable of daisy::io::CFilePath names getPath.

#ifndef OX_IO_IFILEPATH_H
#define OX_IO_IFILEPATH_H

#include "../IUnknown.h"

namespace ox {
namespace io {

//! A file name with its directory aliases resolved, from IFileSystem::resolveAliases.
class IFilePath : public IUnknown
{
public:
    virtual ~IFilePath() {}

    virtual const char* getPath() = 0;
};

} // end namespace io
} // end namespace ox

#endif
