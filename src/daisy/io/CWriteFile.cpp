// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CWriteFile.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CWriteFile.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace io {

CWriteFile::CWriteFile(const char* fileName, bool append)
    : FileSize(0)
{
    Filename = fileName;
    openFile(append);
}

CWriteFile::~CWriteFile()
{
    if (File)
        fclose(File);
}

//! returns how much was read
int CWriteFile::write(const void* buffer, int sizeToWrite)
{
    if (!isOpen())
        return 0;

    return fwrite(buffer, 1, sizeToWrite, File);
}

//! changes position in file, returns true if successful
//! if relativeMovement==true, the pos is changed relative to current pos,
//! otherwise from begin of file
bool CWriteFile::seek(int finalPos, bool relativeMovement)
{
    if (!isOpen())
        return false;

    return fseek(File, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
}

//! returns where in the file we are.
int CWriteFile::getPos()
{
    return ftell(File);
}

//! opens the file
void CWriteFile::openFile(bool append)
{
    if (Filename.size() == 0)
    {
        File = 0;
        return;
    }

    File = fopen(Filename.c_str(), append ? "ab" : "wb");

    if (File)
    {
        // get FileSize

        fseek(File, 0, SEEK_END);
        FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);
    }
}

//! returns name of file
const char* CWriteFile::getFileName()
{
    return Filename.c_str();
}

ox::io::IWriteFile* createWriteFile(const char* fileName, bool append)
{
    CWriteFile* file = new CWriteFile(fileName, append);
    if (file->isOpen())
        return file;

    file->drop();
    return 0;
}

} // end namespace io
} // end namespace daisy
