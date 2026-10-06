// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CReadFile.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CReadFile.h"
#include <sys/stat.h>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace io {

CReadFile::CReadFile(const char* fileName)
    : FileSize(0)
{
    Filename = fileName;
    openFile();
}

CReadFile::~CReadFile()
{
    if (File)
        fclose(File);
}

//! returns how much was read
int CReadFile::read(void* buffer, int sizeToRead)
{
    if (!isOpen())
        return 0;

    return fread(buffer, 1, sizeToRead, File);
}

char* CReadFile::readLine(char* buffer, int size)
{
    if (!isOpen())
        return 0;

    return fgets(buffer, size, File);
}

//! changes position in file, returns true if successful
//! if relativeMovement==true, the pos is changed relative to current pos,
//! otherwise from begin of file
bool CReadFile::seek(int finalPos, bool relativeMovement)
{
    if (!isOpen())
        return false;

    return fseek(File, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
}

//! returns size of file
int CReadFile::getSize()
{
    return FileSize;
}

//! returns where in the file we are.
int CReadFile::getPos()
{
    return ftell(File);
}

long long CReadFile::getModifiedDate()
{
    struct stat64 info;
    fstat64(fileno(File), &info);
    return info.st_mtime;
}

//! opens the file
void CReadFile::openFile()
{
    if (Filename.size() == 0) // bugfix posted by rt
    {
        File = 0;
        return;
    }

    File = fopen(Filename.c_str(), "rb");

    if (File)
    {
        // get FileSize

        fseek(File, 0, SEEK_END);
        FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);
    }
}

//! returns name of file
const char* CReadFile::getFileName()
{
    return Filename.c_str();
}

ox::io::IReadFile* createReadFile(const char* fileName)
{
    CReadFile* file = new CReadFile(fileName);
    if (file->isOpen())
        return file;

    file->drop();
    return 0;
}

} // end namespace io
} // end namespace daisy
