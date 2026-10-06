// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMemoryReadFile.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CMemoryReadFile.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace io {

CMemoryReadFile::CMemoryReadFile(void* memory, int len, const char* fileName, bool d)
    : CMemReadFile(memory, len, d)
{
    Filename = fileName;
}

CMemoryReadFile::~CMemoryReadFile()
{
}

//! returns name of file
const char* CMemoryReadFile::getFileName()
{
    return Filename.c_str();
}

ox::io::IReadFile* createMemoryReadFile(void* memory, int size, const char* fileName, bool deleteMemoryWhenDropped)
{
    CMemoryReadFile* file = new CMemoryReadFile(memory, size, fileName, deleteMemoryWhenDropped);
    return file;
}

} // end namespace io
} // end namespace daisy
