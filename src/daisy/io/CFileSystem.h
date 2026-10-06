// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CFileSystem.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye replaced Irrlicht's list of zip archives with directory aliases, paths into zip archives
// and per-folder file lists.
//
// Paths
// -----
// Every path given to the file system goes through intern_resolveAliases first (except where a
// method says otherwise). An alias is a key such as "$GAME_RESOURCES$" registered with
// addDirectoryAlias; the key includes both dollar signs and matches case-sensitively. Only the
// first "$...$" pair of a path is looked at: when its alias is known, the text before the first '$'
// is dropped, the alias's directory and the rest of the path are joined without adding a separator,
// and the result is resolved again, so aliases may refer to aliases and later aliases are expanded
// too. An unknown alias (or a path without a pair of '$') is returned unchanged, unknown alias
// included; the lookup also stores an empty entry for it. Paths are otherwise handed to the C
// library as they are: '/' separated, relative to the process working directory, case-sensitive.
//
// Archives
// --------
// A path points into an archive when it contains a registered archive extension (".zip" from the
// constructor, more with addZipAlias) after its first character: getZipInPath cuts the path after
// the first such extension ("mods/a.zip/main.lua" gives "mods/a.zip"). Open archives are kept in a
// map keyed by the archive path string exactly as it was passed to getZipReader, which opens an
// archive on first use and keeps it until dropZipReaders or destruction.

#ifndef DAISY_IO_CFILESYSTEM_H
#define DAISY_IO_CFILESYSTEM_H

#include <map>
#include "ox/io/IFileSystem.h"

namespace daisy {
namespace io {

class CZipReader;

const int FILE_SYSTEM_MAX_PATH = 1024;

/*!
    FileSystem which uses normal files, directory aliases and zip archives.
*/
class CFileSystem : public ox::io::IFileSystem
{
public:

    //! constructor
    CFileSystem();

    //! destructor
    virtual ~CFileSystem();

    //! Opens a file for reading: from disk first, else from the archive the path points into.
    virtual ox::io::IReadFile* createAndOpenFile(const char* filename);

    //! Reads a whole file into a memory read file named filename.
    virtual ox::io::IReadFile* readFileIntoMemory(const char* filename);

    //! Opens a file for writing, appending or truncating.
    virtual ox::io::IWriteFile* createAndWriteFile(const char* filename, bool append = false);

    //! Compresses source into target with zlib's default level.
    virtual bool zipDeflateData(unsigned char* target, unsigned int targetSize,
        unsigned char* source, unsigned int sourceSize, unsigned int& written);

    //! Uncompresses a zlib stream from source into target.
    virtual bool zipInflateData(unsigned char* target, unsigned int targetSize,
        unsigned char* source, unsigned int sourceSize, unsigned int& written);

    //! Returns the working directory: the process's, or a path inside an archive.
    virtual const char* getWorkingDirectory();

    //! Sets (or replaces) the directory an alias such as "$GAME_RESOURCES$" stands for.
    virtual void addDirectoryAlias(const char* alias, const char* directory);

    //! Registers a file extension (such as ".zip") whose files are zip archives.
    virtual void addZipAlias(const char* extension);

    //! Drops every open zip reader and forgets them.
    virtual void dropZipReaders();

    //! Returns the directory of an alias, "" for an unknown one.
    virtual const char* getDirectoryFromAlias(const char* alias);

    //! Returns the path with its aliases resolved, for the caller to drop.
    virtual ox::io::IFilePath* resolveAliases(const char* filename);

    //! Creates a directory with mode 0755.
    virtual bool createDirectory(const char* directory);

    //! Lists the files of a folder or of a folder inside an archive.
    virtual ox::io::IFileList* createFileList(const char* filter, const char* directory, ox::io::EFileList mode);

    //! Returns true if a file or directory exists on disk, or with searchArchives inside an open archive.
    virtual bool existFile(const char* filename, bool searchArchives);

    //! Creates a XML Reader from a file.
    virtual ox::io::IXMLReader* createXMLReader(const char* filename);

    //! Creates a XML Reader from a file.
    virtual ox::io::IXMLReader* createXMLReader(ox::io::IReadFile* file);

    //! Creates a XML Writer from a file.
    virtual ox::io::IXMLWriter* createXMLWriter(const char* filename);

    //! Creates a XML Writer from a file.
    virtual ox::io::IXMLWriter* createXMLWriter(ox::io::IWriteFile* file);

    //! Removes a file.
    virtual bool deleteFile(const char* filename);

    //! Renames or moves a file.
    virtual bool renameFile(const char* filename, const char* newName);

    //! Changes the working directory to a folder, or to a path inside an archive.
    virtual bool changeWorkingDirectoryTo(const char* directory);

    //! Returns the archive part of a path, "" when the path does not point into an archive.
    virtual ox::core::CString<char> getZipInPath(const char* filename);

    //! Returns the open zip reader of an archive, opening it on first use; 0 if it cannot be read.
    virtual CZipReader* getZipReader(const char* filename);

private:

    //! Expands the directory aliases of a path; see the top of this file.
    ox::core::CString<char> intern_resolveAliases(const char* filename);

    ox::core::CString<char> WorkingDirectory;
    //! True while the working directory is inside an archive.
    bool WorkingDirectoryInZip;
    //! The archive the working directory is in.
    ox::core::CString<char> WorkingZip;
    //! Alias ("$NAME$") to directory.
    std::map<ox::core::CString<char>, ox::core::CString<char> > Aliases;
    //! Archive path to its open reader.
    std::map<ox::core::CString<char>, CZipReader*> ZipReaders;
    //! Extensions of zip archives (the value is always true).
    std::map<ox::core::CString<char>, bool> ZipExtensions;
};

//! Creates the file system.
ox::io::IFileSystem* createFileSystem();

} // end namespace io
} // end namespace daisy

#endif
