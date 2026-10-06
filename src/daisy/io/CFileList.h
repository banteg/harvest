// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CFileList.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye lists with glob(3) filters and keeps the entries in a std::vector.

#ifndef DAISY_IO_CFILELIST_H
#define DAISY_IO_CFILELIST_H

#include <vector>
#include "ox/io/IFileList.h"
#include "ox/io/IFileSystem.h"
#include "ox/core/CString.h"

namespace daisy {
namespace io {

/*!
    The files of a folder on disk that match a glob(3) pattern; see the constructor.
*/
class CFileList : public ox::io::IFileList
{
public:

    //! Lists the entries of the process working directory that match the glob pattern filter
    //! (for example "*.hmd"); directory only prefixes the full names.
    CFileList(const char* filter, const char* directory, ox::io::EFileList mode);

    //! destructor
    virtual ~CFileList();

    //! Returns the amount of files in the filelist.
    virtual int getFileCount();

    //! Gets the name of a file in the list, based on an index; 0 for a bad index.
    virtual const char* getFileName(int index);

    //! Gets the full name of a file in the list, path included, based on an index.
    virtual const char* getFullFileName(int index);

    //! Returns true if the file is a directory.
    virtual bool isDirectory(int index);

private:

    struct FileEntry
    {
        ox::core::CString<char> Name;
        ox::core::CString<char> FullName;
        int Size;
        bool isDirectory;
    };

    ox::core::CString<char> Path;
    std::vector<FileEntry> Files;
};

} // end namespace io
} // end namespace daisy

#endif
