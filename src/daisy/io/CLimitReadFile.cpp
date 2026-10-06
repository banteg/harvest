// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CLimitReadFile.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CLimitReadFile.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace io {

CLimitReadFile::CLimitReadFile(ox::io::IReadFile* alreadyOpenedFile, int areaSize, const char* name)
    : Filename(name), AreaSize(areaSize), File(alreadyOpenedFile)
{
    if (File)
        File->grab();

    init();
}

void CLimitReadFile::init()
{
    if (!File)
        return;

    AreaStart = File->getPos();
    AreaEnd = AreaStart + AreaSize;
}

CLimitReadFile::~CLimitReadFile()
{
    if (File)
        File->drop();
}

//! returns how much was read
int CLimitReadFile::read(void* buffer, int sizeToRead)
{
    int pos = File->getPos();

    if (pos >= AreaEnd)
        return 0;

    if (pos + sizeToRead >= AreaEnd)
        sizeToRead = AreaEnd - pos;

    return File->read(buffer, sizeToRead);
}

//! changes position in file, returns true if successful
//! if relativeMovement==true, the pos is changed relative to current pos,
//! otherwise from begin of file
bool CLimitReadFile::seek(int finalPos, bool relativeMovement)
{
    int pos = File->getPos();

    if (relativeMovement)
    {
        if (pos + finalPos > AreaEnd)
            finalPos = AreaEnd - pos;
    }
    else
    {
        finalPos += AreaStart;
        if (finalPos > AreaEnd)
            return false;
    }

    return File->seek(finalPos, relativeMovement);
}

//! returns size of file
int CLimitReadFile::getSize()
{
    return AreaSize;
}

//! returns where in the file we are.
int CLimitReadFile::getPos()
{
    return File->getPos() - AreaStart;
}

//! returns name of file
const char* CLimitReadFile::getFileName()
{
    return Filename.c_str();
}

ox::io::IReadFile* createLimitReadFile(const char* fileName, ox::io::IReadFile* alreadyOpenedFile, int areaSize)
{
    return new CLimitReadFile(alreadyOpenedFile, areaSize, fileName);
}

} // end namespace io
} // end namespace daisy
