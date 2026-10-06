// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMemoryReadFile.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye builds it on ox::io::CMemReadFile and only adds the file name.

#ifndef DAISY_IO_CMEMORYREADFILE_H
#define DAISY_IO_CMEMORYREADFILE_H

#include "ox/io/CMemReadFile.h"
#include "ox/core/CString.h"

namespace daisy {
namespace io {

/*!
    Class for reading from memory, with a file name.
*/
class CMemoryReadFile : public ox::io::CMemReadFile
{
public:

    CMemoryReadFile(void* memory, int len, const char* fileName, bool deleteMemoryWhenDropped);

    virtual ~CMemoryReadFile();

    //! returns name of file
    virtual const char* getFileName();

private:

    ox::core::CString<char> Filename;
};

//! Creates a read file over a memory block, which is deleted with the file when wanted.
ox::io::IReadFile* createMemoryReadFile(void* memory, int size, const char* fileName, bool deleteMemoryWhenDropped);

} // end namespace io
} // end namespace daisy

#endif
