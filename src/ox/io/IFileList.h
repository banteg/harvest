// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IFileList.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::io namespace; not the original source.

#ifndef OX_IO_IFILELIST_H
#define OX_IO_IFILELIST_H

#include "../IUnknown.h"

namespace ox {
namespace io {

//! The file list interface, for listing files of a directory.
class IFileList : public IUnknown
{
public:
    virtual ~IFileList() {}

    //! Returns the amount of files in the filelist.
    virtual int getFileCount() = 0;

    //! Gets the name of a file in the list, based on an index.
    virtual const char* getFileName(int index) = 0;

    //! Gets the full name of a file in the list, path included, based on an index.
    virtual const char* getFullFileName(int index) = 0;

    //! Returns whether the file is a directory.
    virtual bool isDirectory(int index) = 0;
};

} // end namespace io
} // end namespace ox

#endif
