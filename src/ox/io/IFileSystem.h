// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::io::CFileSystem; Linux inserts a
// zip-reader-dropping virtual before getDirectoryFromAlias, unlike the Mac build.
// Paths, aliases and archives are described in daisy/io/CFileSystem.h.

#ifndef OX_IO_IFILESYSTEM_H
#define OX_IO_IFILESYSTEM_H

#include "../IUnknown.h"
#include "../core/CString.h"
#include "IReadFile.h"
#include "IWriteFile.h"

// The zip reader has no ox interface: the Linux RTTI derives daisy::io::CZipReader directly from
// ox::IUnknown, and getZipReader hands out the daisy class.
namespace daisy {
namespace io {
class CZipReader;
} // end namespace io
} // end namespace daisy

namespace ox {
namespace io {

class IFilePath;
class IFileList;
class IXMLReader;
class IXMLWriter;

//! What createFileList lists from a folder. Archive listings ignore it.
enum EFileList
{
    //! Files and directories.
    EFL_ALL = 0,
    //! Files only.
    EFL_FILES,
    //! Directories only.
    EFL_DIRECTORIES
};

class IFileSystem : public IUnknown
{
public:
    virtual ~IFileSystem() {};
    virtual IReadFile* createAndOpenFile(const char* filename) = 0;
    virtual IReadFile* readFileIntoMemory(const char* filename) = 0;
    virtual IWriteFile* createAndWriteFile(const char* filename, bool append) = 0;
    //! Compresses source into target as one zlib stream; written receives the compressed size.
    virtual bool zipDeflateData(unsigned char* target, unsigned int targetSize,
        unsigned char* source, unsigned int sourceSize, unsigned int& written) = 0;
    //! Uncompresses a zlib stream from source into target; written receives the uncompressed size.
    virtual bool zipInflateData(unsigned char* target, unsigned int targetSize,
        unsigned char* source, unsigned int sourceSize, unsigned int& written) = 0;
    virtual const char* getWorkingDirectory() = 0;
    virtual void addDirectoryAlias(const char* alias, const char* directory) = 0;
    virtual void addZipAlias(const char* extension) = 0;
    // Descriptive name: the stripped Linux slot drops every open zip reader and forgets them.
    virtual void dropZipReaders() = 0;
    virtual const char* getDirectoryFromAlias(const char* alias) = 0;
    virtual IFilePath* resolveAliases(const char* filename) = 0;
    virtual bool createDirectory(const char* directory) = 0;
    virtual IFileList* createFileList(const char* filter, const char* directory, EFileList mode) = 0;
    //! searchArchives also looks inside zip archives that are already open.
    virtual bool existFile(const char* filename, bool searchArchives) = 0;
    virtual IXMLReader* createXMLReader(const char* filename) = 0;
    virtual IXMLReader* createXMLReader(IReadFile* file) = 0;
    virtual IXMLWriter* createXMLWriter(const char* filename) = 0;
    virtual IXMLWriter* createXMLWriter(IWriteFile* file) = 0;
    virtual bool deleteFile(const char* filename) = 0;
    virtual bool renameFile(const char* filename, const char* newName) = 0;
    virtual bool changeWorkingDirectoryTo(const char* directory) = 0;
    virtual core::CString<char> getZipInPath(const char* filename) = 0;
    virtual daisy::io::CZipReader* getZipReader(const char* filename) = 0;
};

} // end namespace io
} // end namespace ox

#endif
