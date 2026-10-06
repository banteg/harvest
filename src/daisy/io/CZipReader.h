// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CZipReader.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye keeps the entries in a std::vector, reads the local headers field by field, finds the
// sizes of streamed entries from their data descriptor and looks files up by folder and name.

#ifndef DAISY_IO_CZIPREADER_H
#define DAISY_IO_CZIPREADER_H

#include <vector>
#include "ox/IUnknown.h"
#include "ox/io/IReadFile.h"
#include "ox/core/CString.h"

namespace daisy {
namespace io {

const short ZIP_FILE_ENCRYPTED = 0x0001; // set if the file is encrypted
const short ZIP_INFO_IN_DATA_DESCRIPTOR = 0x0008; // the fields crc-32, compressed size and
                                                  // uncompressed size are set to zero in the local
                                                  // header and follow the data instead

#if defined(__GNUC__)
#   define PACK_STRUCT __attribute__((packed))
#else
#   error compiler not supported
#endif

struct SZIPFileDataDescriptor
{
    int CRC32;
    int CompressedSize;
    int UncompressedSize;
} PACK_STRUCT;

//! A zip local file header, without the file name and extra field that follow it.
struct SZIPFileHeader
{
    int Sig;
    short VersionToExtract;
    short GeneralBitFlag;
    short CompressionMethod;
    short LastModFileTime;
    short LastModFileDate;
    SZIPFileDataDescriptor DataDescriptor;
    short FilenameLength;
    short ExtraFieldLength;
} PACK_STRUCT;

#undef PACK_STRUCT

//! One file or folder of the archive.
struct SZipFileEntry
{
    //! The stored name, '\\' turned into '/' (lower case with ignoreCase). Without ignorePaths it
    //! is cut down to simpleFileName, as in Irrlicht.
    ox::core::CString<char> zipFileName;
    //! The name after the last '/'; empty for a folder entry ("dir/").
    ox::core::CString<char> simpleFileName;
    //! The whole stored name when it contains a '/' ("dir/file.lua"), else empty.
    ox::core::CString<char> path;
    int fileDataPosition; // position of compressed data in file
    SZIPFileHeader header;

    bool operator<(const SZipFileEntry& other) const
    {
        return simpleFileName < other.simpleFileName;
    }

    bool operator==(const SZipFileEntry& other) const
    {
        return simpleFileName == other.simpleFileName;
    }
};

/*!
    Zip file Reader written April 2002 by N.Gebhardt.
    Reads the local headers from the start of the archive (the central directory is never read)
    and opens stored and deflated entries.
*/
class CZipReader : public ox::IUnknown
{
public:
    CZipReader(ox::io::IReadFile* file, bool ignoreCase, bool ignorePaths);
    virtual ~CZipReader();

    //! opens a file by file name
    virtual ox::io::IReadFile* openFile(const char* filename);

    //! opens a file by index
    ox::io::IReadFile* openFile(int index);

    //! returns count of files in archive
    int getFileCount();

    //! returns data of file
    const SZipFileEntry* getFileInfo(int index) const;

    //! returns fileindex
    int findFile(const char* filename);

    //! Returns true for "" and when an entry's path equals directory. See the definition.
    bool directoryExists(const char* directory);

private:
    //! scans for a local header, returns false if there is no more local file header.
    bool scanLocalHeader();

    //! splits filename from zip file into useful filenames and paths
    void extractFilename(SZipFileEntry* entry);

    //! deletes the path from a filename
    void deletePathFromFilename(ox::core::CString<char>& filename);

    ox::io::IReadFile* File;

    std::vector<SZipFileEntry> FileList;

    bool IgnoreCase;
    bool IgnorePaths;
};

} // end namespace io
} // end namespace daisy

#endif
