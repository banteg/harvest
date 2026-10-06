// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IReadFile.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::io namespace; not the original source. Virtual order follows the
// Mac and Linux 1.18 vtables; readLine and getModifiedDate are Oxeye additions.

#ifndef OX_IO_IREADFILE_H
#define OX_IO_IREADFILE_H

#include "../IUnknown.h"

namespace ox {
namespace io {

//! Interface providing read access to a file.
class IReadFile : public IUnknown
{
public:
    virtual ~IReadFile() {};

    //! Reads an amount of bytes from the file. Returns how many bytes were read.
    virtual int read(void* buffer, int sizeToRead) = 0;

    //! Reads one line like fgets; returns buffer, or 0 at the end or on error. Pointer-sized:
    //! daisy::io::CReadFile tail-calls fgets.
    virtual char* readLine(char* buffer, int size) = 0;

    //! Changes the position in the file. Returns true on success.
    virtual bool seek(int finalPos, bool relativeMovement = false) = 0;

    virtual int getSize() = 0;
    virtual int getPos() = 0;
    virtual const char* getFileName() = 0;

    // Return type provisional: the Mac i386 build returns it in edx:eax, so it is 64-bit.
    virtual long long getModifiedDate() = 0;
};

} // end namespace io
} // end namespace ox

#endif
