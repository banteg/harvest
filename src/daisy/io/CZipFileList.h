// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_IO_CZIPFILELIST_H
#define DAISY_IO_CZIPFILELIST_H

#include <vector>
#include "ox/io/IFileList.h"
#include "ox/core/CString.h"

namespace daisy {
namespace io {

class CZipReader;

//! The files of one folder of a zip archive, for IFileSystem::createFileList while the working
//! directory is inside an archive.
class CZipFileList : public ox::io::IFileList
{
public:
    //! Lists the entries of reader directly in directory ("" for the top level, else "dir/" with
    //! the trailing slash), filtered by filter. See the definition for the rules.
    CZipFileList(CZipReader* reader, const char* filter, const char* directory);

    virtual ~CZipFileList();

    //! Returns the amount of files in the filelist.
    virtual int getFileCount();

    //! Gets the name of a file in the list, based on an index; 0 for a negative index.
    virtual const char* getFileName(int index);

    //! Gets the full name of a file in the list, path included, based on an index.
    virtual const char* getFullFileName(int index);

    //! Returns whether the file is a directory: always false for archive lists.
    virtual bool isDirectory(int index);

private:
    struct FileEntry
    {
        ox::core::CString<char> Name;
        ox::core::CString<char> FullName;
        int Size;
        bool isDirectory;
    };

    std::vector<FileEntry> Files;
};

} // end namespace io
} // end namespace daisy

#endif
