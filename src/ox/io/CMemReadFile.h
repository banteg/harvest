// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMemoryReadFile.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest; not the original source. Oxeye's version drops the file name, defines
// readLine, getFileName and getModifiedDate inline (Linux keeps them as COMDAT copies), and adds
// getCurrentPointer and getRemainingSize, which IReadFile does not declare.

#ifndef OX_IO_CMEMREADFILE_H
#define OX_IO_CMEMREADFILE_H

#include "IReadFile.h"

namespace ox {
namespace io {

//! Reads from a block of memory as if it were a file.
class CMemReadFile : public IReadFile
{
public:
    CMemReadFile(void* memory, int len, bool deleteMemoryWhenDropped);
    virtual ~CMemReadFile();

    virtual int read(void* buffer, int sizeToRead);
    virtual char* readLine(char* buffer, int size) { return 0; }
    virtual bool seek(int finalPos, bool relativeMovement = false);
    virtual int getSize();
    virtual int getPos();
    virtual const char* getFileName() { return "MemFile"; }
    virtual long long getModifiedDate() { return 0; }
    virtual void* getCurrentPointer();
    virtual int getRemainingSize();

private:
    void* Buffer;
    unsigned int Len;
    unsigned int Pos;
    bool deleteMemoryWhenDropped;
};

} // end namespace io
} // end namespace ox

#endif
