// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CReadFile.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye adds readLine and getModifiedDate.

#ifndef DAISY_IO_CREADFILE_H
#define DAISY_IO_CREADFILE_H

#include <stdio.h>
#include "ox/io/IReadFile.h"
#include "ox/core/CString.h"

namespace daisy {
namespace io {

/*!
    Class for reading a real file from disk.
*/
class CReadFile : public ox::io::IReadFile
{
public:

    CReadFile(const char* fileName);

    virtual ~CReadFile();

    //! returns how much was read
    virtual int read(void* buffer, int sizeToRead);

    //! Reads one line, newline included, like fgets.
    virtual char* readLine(char* buffer, int size);

    //! changes position in file, returns true if successful
    //! if relativeMovement==true, the pos is changed relative to current pos,
    //! otherwise from begin of file
    virtual bool seek(int finalPos, bool relativeMovement = false);

    //! returns size of file
    virtual int getSize();

    //! returns where in the file we are.
    virtual int getPos();

    //! returns name of file
    virtual const char* getFileName();

    //! Returns the file's modification time in seconds since the epoch.
    virtual long long getModifiedDate();

    //! returns if file is open
    bool isOpen()
    {
        return File != 0;
    }

private:

    //! opens the file
    void openFile();

    ox::core::CString<char> Filename;
    FILE* File;
    int FileSize;
};

//! Opens a file on disk for reading ("rb"); 0 if it cannot be opened.
ox::io::IReadFile* createReadFile(const char* fileName);

} // end namespace io
} // end namespace daisy

#endif
