// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CZipFileList.h"
#include "CZipReader.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace io {

//! Lists the entries whose folder part (path minus simpleFileName, "dir/" or "") equals directory
//! exactly and case sensitively, in the reader's order (sorted by simpleFileName). Entries of
//! subfolders are not listed and folders never show up as directories; a folder entry "dir/"
//! itself is listed under "dir/" with an empty name. A filter that starts with '*' and has more
//! after it ("*.lua") keeps the names ending with the rest; any other filter lists everything.
/** Name is the simpleFileName, FullName the entry's path (the whole stored name, folders
    included, or empty for top-level entries) and Size the uncompressed size. */
CZipFileList::CZipFileList(CZipReader* reader, const char* filter, const char* directory)
{
    ox::core::CString<char> extension = filter;

    if (extension.findFirst('*') == 0 && extension.size() >= 2)
    {
        extension = extension.subStringToEnd(extension.findFirst('*') + 1);

        int count = reader->getFileCount();
        if (count > 0 && reader)
        {
            for (int i = 0; i < count; ++i)
            {
                const SZipFileEntry* entry = reader->getFileInfo(i);
                if (!entry)
                    continue;

                ox::core::CString<char> folder = entry->path.subString(0,
                    entry->path.size() - entry->simpleFileName.size());
                if (folder == directory && entry->simpleFileName.endsWith(extension))
                {
                    FileEntry file;
                    file.Name = entry->simpleFileName;
                    file.FullName = entry->path;
                    file.Size = entry->header.DataDescriptor.UncompressedSize;
                    file.isDirectory = false;
                    Files.push_back(file);
                }
            }
        }
    }
    else
    {
        int count = reader->getFileCount();
        if (count > 0 && reader)
        {
            for (int i = 0; i < count; ++i)
            {
                const SZipFileEntry* entry = reader->getFileInfo(i);
                if (!entry)
                    continue;

                ox::core::CString<char> folder = entry->path.subString(0,
                    entry->path.size() - entry->simpleFileName.size());
                if (folder == directory)
                {
                    FileEntry file;
                    file.Name = entry->simpleFileName;
                    file.FullName = entry->path;
                    file.Size = entry->header.DataDescriptor.UncompressedSize;
                    file.isDirectory = false;
                    Files.push_back(file);
                }
            }
        }
    }
}

CZipFileList::~CZipFileList()
{
}

//! Returns the amount of files in the filelist.
int CZipFileList::getFileCount()
{
    return Files.size();
}

// The index checks let index == getFileCount() through, one past the end, as in both builds.

//! Gets the name of a file in the list, based on an index.
const char* CZipFileList::getFileName(int index)
{
    if (index < 0 || index > (int)Files.size())
        return 0;

    return Files[index].Name.c_str();
}

//! Gets the full name of a file in the list, path included, based on an index.
const char* CZipFileList::getFullFileName(int index)
{
    if (index < 0 || index > (int)Files.size())
        return 0;

    return Files[index].FullName.c_str();
}

//! Returns whether the file is a directory.
bool CZipFileList::isDirectory(int index)
{
    if (index < 0 || index > (int)Files.size())
        return false;

    return Files[index].isDirectory;
}

} // end namespace io
} // end namespace daisy
