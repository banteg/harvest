// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Only the virtual prefix needed by CHelpIO is declared here. Linux inserts a
// path-cache-clearing virtual before getDirectoryFromAlias, unlike the Mac build.

#ifndef OX_IO_IFILESYSTEM_H
#define OX_IO_IFILESYSTEM_H

#include "../IUnknown.h"
#include "../core/CString.h"
#include "IReadFile.h"
#include "IWriteFile.h"

namespace ox {
namespace io {

class IFilePath;
class IFileList;
// Only the enum's ABI type is needed here; its enumerators are not recovered yet.
enum EFileList {};

class IFileSystem : public IUnknown
{
public:
    virtual ~IFileSystem() {};
    virtual IReadFile* createAndOpenFile(const char* filename) = 0;
    virtual IReadFile* readFileIntoMemory(const char* filename) = 0;
    virtual IWriteFile* createAndWriteFile(const char* filename, bool append) = 0;
    virtual bool zipDeflateData(unsigned char* source, unsigned int sourceSize,
        unsigned char* target, unsigned int targetSize, unsigned int& written) = 0;
    virtual bool zipInflateData(unsigned char* source, unsigned int sourceSize,
        unsigned char* target, unsigned int targetSize, unsigned int& written) = 0;
    virtual const char* getWorkingDirectory() = 0;
    virtual void addDirectoryAlias(const char* alias, const char* directory) = 0;
    virtual void addZipAlias(const char* filename) = 0;
    // Descriptive name: the stripped Linux slot clears cached IFilePath references.
    virtual void clearCachedFilePaths() = 0;
    virtual const char* getDirectoryFromAlias(const char* alias) = 0;
    virtual IFilePath* resolveAliases(const char* filename) = 0;
    virtual bool createDirectory(const char* directory) = 0;
    virtual IFileList* createFileList(const char* filter, const char* directory, EFileList mode) = 0;
    virtual bool existFile(const char* filename, bool ignoreArchives) = 0;
};

} // end namespace io
} // end namespace ox

#endif
