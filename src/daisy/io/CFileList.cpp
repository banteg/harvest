// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CFileList.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CFileList.h"
#include <glob.h>
#include <string.h>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace io {

CFileList::~CFileList()
{
}

int CFileList::getFileCount()
{
    return Files.size();
}

// The index checks accept index == getFileCount(), as in Irrlicht 0.7.
const char* CFileList::getFileName(int index)
{
    if (index < 0 || index > (int)Files.size())
        return 0;

    return Files[index].Name.c_str();
}

//! Gets the full name of a file in the list, path included, based on an index.
//! The full name is the directory given to the constructor (aliases unresolved), a '/' unless it
//! already ends with one, and the name; it is rebuilt on every call.
const char* CFileList::getFullFileName(int index)
{
    if (index < 0 || index > (int)Files.size())
        return 0;

    FileEntry& entry = Files[index];
    entry.FullName = ox::core::CString<char>(Path);
    if (!entry.FullName.endsWith("/"))
        entry.FullName.append("/");
    entry.FullName.append(entry.Name);

    return entry.FullName.c_str();
}

bool CFileList::isDirectory(int index)
{
    if (index < 0 || index > (int)Files.size())
        return false;

    return Files[index].isDirectory;
}

//! glob(3) runs in the process working directory, which CFileSystem::createFileList has changed
//! to the listed folder. The pattern syntax is the shell's ('*', '?', "[...]"), it is
//! case-sensitive, and names starting with '.' only match a pattern that starts with '.'.
//! GLOB_MARK appends '/' to directory names: that slash stays in the names and decides
//! isDirectory. The entries keep glob's order, which sorts the names with strcoll, so it follows
//! the LC_COLLATE locale ("C": byte order, upper case before lower case).
CFileList::CFileList(const char* filter, const char* directory, ox::io::EFileList mode)
    : Path(directory)
{
    FileEntry entry;
    ox::core::CString<char> pattern(filter);

    glob_t files;
    glob(filter, GLOB_MARK, 0, &files);

    for (int i = 0; i < files.gl_pathc; ++i)
    {
        entry.Name = files.gl_pathv[i];
        entry.Size = 0;
        entry.isDirectory = files.gl_pathv[i][strlen(files.gl_pathv[i]) - 1] == '/';

        switch (mode)
        {
        case ox::io::EFL_ALL:
            Files.push_back(entry);
            break;
        case ox::io::EFL_FILES:
            if (!entry.isDirectory)
                Files.push_back(entry);
            break;
        case ox::io::EFL_DIRECTORIES:
            if (entry.isDirectory)
                Files.push_back(entry);
            break;
        }
    }

    globfree(&files);
}

} // end namespace io
} // end namespace daisy
