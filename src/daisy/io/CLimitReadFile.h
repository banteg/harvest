// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CLimitReadFile.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye adds readLine and getModifiedDate, which do nothing here.

#ifndef DAISY_IO_CLIMITREADFILE_H
#define DAISY_IO_CLIMITREADFILE_H

#include "ox/io/IReadFile.h"
#include "ox/core/CString.h"

namespace daisy {
namespace io {

/*! this is a read file, which is limited to some boundaries,
    so that it may only start from a certain file position
    and may only read until a certain file position.
    This can be useful, for example for reading uncompressed files
    in an archive (zip).
*/
class CLimitReadFile : public ox::io::IReadFile
{
public:

    CLimitReadFile(ox::io::IReadFile* alreadyOpenedFile, int areaSize, const char* name);

    virtual ~CLimitReadFile();

    //! returns how much was read
    virtual int read(void* buffer, int sizeToRead);

    virtual char* readLine(char* buffer, int size) { return 0; }

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

    virtual long long getModifiedDate() { return 0; }

private:

    void init();

    ox::core::CString<char> Filename;
    int AreaSize;
    int AreaStart;
    int AreaEnd;
    ox::io::IReadFile* File;
};

//! Creates a read file over areaSize bytes of an open file, starting at its current position.
ox::io::IReadFile* createLimitReadFile(const char* fileName, ox::io::IReadFile* alreadyOpenedFile, int areaSize);

} // end namespace io
} // end namespace daisy

#endif
