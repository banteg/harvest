// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CWriteFile.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye adds getStdioFile.

#ifndef DAISY_IO_CWRITEFILE_H
#define DAISY_IO_CWRITEFILE_H

#include <stdio.h>
#include "ox/io/IWriteFile.h"
#include "ox/core/CString.h"

namespace daisy {
namespace io {

/*!
    Instance of IWriteFile writing to a real file on disk.
*/
class CWriteFile : public ox::io::IWriteFile
{
public:

    CWriteFile(const char* fileName, bool append);

    virtual ~CWriteFile();

    //! Reads an amount of bytes from the file.
    virtual int write(const void* buffer, int sizeToWrite);

    //! Changes position in file, returns true if successful.
    virtual bool seek(int finalPos, bool relativeMovement = false);

    //! Returns the current position in the file.
    virtual int getPos();

    //! Returns name of file.
    virtual const char* getFileName();

    //! Returns the C library stream.
    virtual FILE* getStdioFile() { return File; }

    //! returns if file is open
    bool isOpen()
    {
        return File != 0;
    }

private:

    //! opens the file
    void openFile(bool append);

    ox::core::CString<char> Filename;
    FILE* File;
    int FileSize;
};

//! Opens a file on disk for writing ("ab" to append, else "wb"); 0 if it cannot be opened.
ox::io::IWriteFile* createWriteFile(const char* fileName, bool append);

} // end namespace io
} // end namespace daisy

#endif
