// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CZipReader.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
//
// How an archive is read (mods ship as harvestClientData/mods/*.zip):
// - Only the local file headers are scanned, from offset 0 until a record without the local header
//   signature "PK\3\4"; the central directory is never read. Archive comments, zip64 and
//   encryption are not supported.
// - Entries that stream their sizes (general purpose bit 3) are skipped by searching byte by byte
//   for the data descriptor signature "PK\7\8"; the descriptor's sizes then replace the zeros of the
//   local header. Other entries are skipped by their compressed size.
// - Method 0 (stored) entries open as a CLimitReadFile window over the archive file, so they
//   share its file position. Method 8 (deflated) entries are inflated whole into memory (raw
//   deflate, windowBits -15) and open as a CMemoryReadFile. Other methods log an error and open
//   nothing.
// - Names are case sensitive and '\\' becomes '/'. CFileSystem creates its readers without
//   ignoreCase and ignorePaths.

#include "CZipReader.h"
#include "CLimitReadFile.h"
#include "CMemoryReadFile.h"
#include "daisy/os.h"
#include "ox/io/CHelpIO.h"
#include "ox/core/CStringFunctions.h"
#include "ox/algo/CArrayFunctions.h"
#include <string.h>
#include <zlib.h>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace io {

//! The local file header signature, "PK\3\4".
static const int ZIP_LOCAL_HEADER_SIGNATURE = 0x04034b50;

CZipReader::CZipReader(ox::io::IReadFile* file, bool ignoreCase, bool ignorePaths)
    : File(file), IgnoreCase(ignoreCase), IgnorePaths(ignorePaths)
{
    if (File)
    {
        File->grab();

        // scan local headers
        while (scanLocalHeader());

        // listings come out in this order
        ox::algo::sort(FileList.begin(), FileList.end());
    }
}

//! scans for a local header, returns false if there is no more local file header.
bool CZipReader::scanLocalHeader()
{
    char tmp[1024];

    SZipFileEntry entry;
    entry.fileDataPosition = 0;
    memset(&entry.header, 0, sizeof(SZIPFileHeader));

    // field by field, little endian whatever the host
    entry.header.Sig = ox::io::CHelpIO::readInt(File);
    entry.header.VersionToExtract = ox::io::CHelpIO::readShort(File);
    entry.header.GeneralBitFlag = ox::io::CHelpIO::readShort(File);
    entry.header.CompressionMethod = ox::io::CHelpIO::readShort(File);
    entry.header.LastModFileTime = ox::io::CHelpIO::readShort(File);
    entry.header.LastModFileDate = ox::io::CHelpIO::readShort(File);
    entry.header.DataDescriptor.CRC32 = ox::io::CHelpIO::readInt(File);
    entry.header.DataDescriptor.CompressedSize = ox::io::CHelpIO::readInt(File);
    entry.header.DataDescriptor.UncompressedSize = ox::io::CHelpIO::readInt(File);
    entry.header.FilenameLength = ox::io::CHelpIO::readShort(File);
    entry.header.ExtraFieldLength = ox::io::CHelpIO::readShort(File);

    if (entry.header.Sig != ZIP_LOCAL_HEADER_SIGNATURE)
        return false; // local file headers end here.

    // read filename
    entry.zipFileName.reserve(entry.header.FilenameLength + 2);
    File->read(tmp, entry.header.FilenameLength);
    tmp[entry.header.FilenameLength] = 0x0;
    entry.zipFileName = tmp;
    entry.zipFileName.replace('\\', '/');

    extractFilename(&entry);

    // move forward length of extra field.
    if (entry.header.ExtraFieldLength)
        File->seek(entry.header.ExtraFieldLength, true);

    // store position in file
    entry.fileDataPosition = File->getPos();

    if (entry.header.GeneralBitFlag & ZIP_INFO_IN_DATA_DESCRIPTOR)
    {
        // The sizes follow the data: search for the descriptor signature "PK\7\8". A mismatch
        // restarts the search at the next byte without retrying it as the first signature byte.
        const char SEQUENCE[4] = { 'P', 'K', 7, 8 };
        int i = 0;
        do
        {
            char c = 0;
            if (File->read(&c, 1) != 1)
                break;
            if (c != SEQUENCE[i++])
                i = 0;
        }
        while (i < 4);

        entry.header.DataDescriptor.CRC32 = ox::io::CHelpIO::readInt(File);
        entry.header.DataDescriptor.CompressedSize = ox::io::CHelpIO::readInt(File);
        entry.header.DataDescriptor.UncompressedSize = ox::io::CHelpIO::readInt(File);
    }
    else
    {
        // move forward length of data
        File->seek(entry.header.DataDescriptor.CompressedSize, true);
    }

    FileList.push_back(entry);

    return true;
}

CZipReader::~CZipReader()
{
    if (File)
        File->drop();
}

//! splits filename from zip file into useful filenames and paths
void CZipReader::extractFilename(SZipFileEntry* entry)
{
    int lorfn = entry->header.FilenameLength; // length of real file name

    if (!lorfn)
        return;

    if (IgnoreCase)
        ox::core::CStringFunctions::ansiMakeLower(entry->zipFileName);

    const char* p = entry->zipFileName.c_str() + lorfn;

    // search a slash or the start, from the end
    while (*p != '/' && p != entry->zipFileName.c_str())
    {
        --p;
        --lorfn;
    }

    bool thereIsAPath = p != entry->zipFileName.c_str();

    if (thereIsAPath)
    {
        // there is a path
        ++p;
        ++lorfn;
    }

    entry->simpleFileName = p;
    entry->path = "";

    // Unlike Irrlicht, the path keeps the whole name, file part included.
    if (thereIsAPath)
        entry->path.append(entry->zipFileName);

    // inverted as in Irrlicht: the name loses its folders when paths are NOT ignored
    if (!IgnorePaths)
        entry->zipFileName = entry->simpleFileName;
}

//! opens a file by file name
ox::io::IReadFile* CZipReader::openFile(const char* filename)
{
    int index = findFile(filename);

    if (index != -1)
        return openFile(index);

    return 0;
}

//! Returns the index of the entry whose folder and name match filename, or -1.
/** One leading '/' is ignored. "dir/file" matches the entry stored as "dir/file" (path equal,
    simpleFileName equal) and "file" matches a top-level "file" only. ignoreCase lowers only the
    name part of the request and ignorePaths strips it to its last '/' or '\\' part; the path is
    compared as given either way. The search is linear: the list is sorted for listings only. */
int CZipReader::findFile(const char* simpleFilename)
{
    ox::core::CString<char> fileName = simpleFilename;
    if (fileName[0] == '/')
        fileName = fileName.subStringToEnd(1);

    SZipFileEntry entry;
    int lastSlash = fileName.findLast('/');
    if (lastSlash != -1)
    {
        entry.simpleFileName = fileName.subStringToEnd(lastSlash + 1);
        entry.path = fileName;
    }
    else
        entry.simpleFileName = fileName;

    if (IgnoreCase)
        ox::core::CStringFunctions::ansiMakeLower(entry.simpleFileName);

    if (IgnorePaths)
        deletePathFromFilename(entry.simpleFileName);

    int result = -1;
    for (unsigned int i = 0; i < FileList.size(); ++i)
    {
        if (FileList[i].simpleFileName == entry.simpleFileName)
        {
            if (FileList[i].path == entry.path)
            {
                result = i;
                break;
            }
        }
    }

    return result;
}

//! opens a file by index
ox::io::IReadFile* CZipReader::openFile(int index)
{
    //0 - The file is stored (no compression)
    //1 - The file is Shrunk
    //2 - The file is Reduced with compression factor 1
    //3 - The file is Reduced with compression factor 2
    //4 - The file is Reduced with compression factor 3
    //5 - The file is Reduced with compression factor 4
    //6 - The file is Imploded
    //7 - Reserved for Tokenizing compression algorithm
    //8 - The file is Deflated
    //9 - Reserved for enhanced Deflating
    //10 - PKWARE Date Compression Library Imploding

    switch (FileList[index].header.CompressionMethod)
    {
    case 0: // no compression
        {
            File->seek(FileList[index].fileDataPosition);
            return createLimitReadFile(FileList[index].simpleFileName.c_str(), File,
                FileList[index].header.DataDescriptor.UncompressedSize);
        }
    case 8:
        {
            unsigned int uncompressedSize = FileList[index].header.DataDescriptor.UncompressedSize;
            unsigned int compressedSize = FileList[index].header.DataDescriptor.CompressedSize;

            void* pBuf = new char[uncompressedSize];
            if (!pBuf)
            {
                os::Printer::log("Not enough memory for decompressing",
                    FileList[index].simpleFileName.c_str(), ox::event::ELL_ERROR);
                return 0;
            }

            char* pcData = new char[compressedSize];
            if (!pcData)
            {
                os::Printer::log("Not enough memory for decompressing",
                    FileList[index].simpleFileName.c_str(), ox::event::ELL_ERROR);
                return 0;
            }

            File->seek(FileList[index].fileDataPosition);
            File->read(pcData, compressedSize);

            // Setup the inflate stream.
            z_stream stream;
            int err;

            stream.next_in = (Bytef*)pcData;
            stream.avail_in = (uInt)compressedSize;
            stream.next_out = (Bytef*)pBuf;
            stream.avail_out = uncompressedSize;
            stream.zalloc = (alloc_func)0;
            stream.zfree = (free_func)0;

            // Perform inflation. wbits < 0 indicates no zlib header inside the data.
            err = inflateInit2(&stream, -MAX_WBITS);
            if (err == Z_OK)
            {
                // the result of inflate is ignored, as in Irrlicht
                err = inflate(&stream, Z_FINISH);
                inflateEnd(&stream);
                if (err == Z_STREAM_END)
                    err = Z_OK;

                err = Z_OK;
                inflateEnd(&stream);
            }

            delete[] pcData;

            if (err != Z_OK)
            {
                os::Printer::log("Error decompressing", FileList[index].simpleFileName.c_str(),
                    ox::event::ELL_ERROR);
                delete [] (char*)pBuf;
                return 0;
            }
            else
                return createMemoryReadFile(pBuf, uncompressedSize,
                    FileList[index].simpleFileName.c_str(), true);
        }
        break;
    default:
        os::Printer::log("file has unsupported compression method.",
            FileList[index].simpleFileName.c_str(), ox::event::ELL_ERROR);
        return 0;
    };
}

//! returns count of files in archive
int CZipReader::getFileCount()
{
    return FileList.size();
}

//! returns data of file
const SZipFileEntry* CZipReader::getFileInfo(int index) const
{
    return &FileList[index];
}

//! deletes the path from a filename
void CZipReader::deletePathFromFilename(ox::core::CString<char>& filename)
{
    // delete path from filename
    const char* p = filename.c_str() + filename.size();

    // search a slash or the start.
    while (*p != '/' && *p != '\\' && p != filename.c_str())
        --p;

    ox::core::CString<char> newName;

    if (p != filename.c_str())
    {
        ++p;
        filename = p;
    }
}

//! Returns true for "" and when an entry's path equals directory.
/** The comparison uses directory as given, not the "directory/" built first, and paths are
    whole entry names ("dir/" for a folder entry, "dir/file" for a file). The loop never advances
    its iterator, so it only terminates when the first entry matches or the list is empty; any
    other request hangs. */
bool CZipReader::directoryExists(const char* directory)
{
    ox::core::CString<char> dir = directory;
    if (dir == "")
        return true;

    dir.append("/");

    std::vector<SZipFileEntry>::iterator it = FileList.begin();
    while (it != FileList.end())
    {
        if (it->path == directory)
            return true;
    }

    return false;
}

} // end namespace io
} // end namespace daisy
