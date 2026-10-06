// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CFileSystem.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CFileSystem.h"
#include "CFilePath.h"
#include "CFileList.h"
#include "CReadFile.h"
#include "CWriteFile.h"
#include "CMemoryReadFile.h"
#include "CZipReader.h"
#include "CZipFileList.h"
#include "CTextReader.h"
#include "CXMLReader.h"
#include "CXMLWriter.h"
#include "daisy/os.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <zlib.h>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace io {

//! constructor
CFileSystem::CFileSystem()
{
    char tmp[FILE_SYSTEM_MAX_PATH];
    getcwd(tmp, FILE_SYSTEM_MAX_PATH);
    WorkingDirectory = ox::core::CString<char>(tmp);

    WorkingDirectoryInZip = false;
    // CString(int): the zip path starts as "0", not "".
    WorkingZip = 0;

    ZipExtensions[".zip"] = true;
}

//! destructor
CFileSystem::~CFileSystem()
{
    for (std::map<ox::core::CString<char>, CZipReader*>::iterator it = ZipReaders.begin();
        it != ZipReaders.end(); ++it)
    {
        if (it->second)
            it->second->drop();
    }
}

//! Opens a file for reading. The alias-resolved path is tried on disk first. Otherwise, when the
//! path as given (aliases unresolved) points into an archive, the archive is opened through
//! getZipReader under that unresolved name and the rest of the path after the archive and its
//! '/' is opened inside it.
ox::io::IReadFile* CFileSystem::createAndOpenFile(const char* filename)
{
    ox::core::CString<char> path = intern_resolveAliases(filename);
    ox::io::IReadFile* file = createReadFile(path.c_str());

    if (!file)
    {
        ox::core::CString<char> zipPath = getZipInPath(filename);
        if (zipPath.size() > 0)
        {
            CZipReader* reader = getZipReader(zipPath.c_str());
            if (reader)
            {
                ox::core::CString<char> inZip(filename);
                inZip = inZip.subStringToEnd(zipPath.size() + 1);
                file = reader->openFile(inZip.c_str());
            }
        }
    }

    return file;
}

ox::core::CString<char> CFileSystem::intern_resolveAliases(const char* filename)
{
    ox::core::CString<char> path(filename);
    ox::core::CString<char> result(filename);

    int start = path.findFirst('$');
    if (start != -1)
    {
        int end = path.findNext('$', start + 1);
        if (end != -1)
        {
            ox::core::CString<char> alias = path.subString(start, end - start + 1);
            ox::core::CString<char> directory = Aliases[alias];
            if (directory.size() > 0)
            {
                directory.append(path.subStringToEnd(end + 1));
                result = intern_resolveAliases(directory.c_str());
            }
        }
    }

    return result;
}

//! Opens a file for write access.
ox::io::IWriteFile* CFileSystem::createAndWriteFile(const char* filename, bool append)
{
    ox::core::CString<char> path = intern_resolveAliases(filename);
    if (path.size() > 0)
        return createWriteFile(path.c_str(), append);

    return 0;
}

//! Save games are compressed with this: one zlib stream (with header) at Z_DEFAULT_COMPRESSION.
//! On a zlib error after deflateInit the stream is not ended.
bool CFileSystem::zipDeflateData(unsigned char* target, unsigned int targetSize,
    unsigned char* source, unsigned int sourceSize, unsigned int& written)
{
    if (!source || !target)
        return false;

    z_stream stream;
    stream.next_in = source;
    stream.avail_in = sourceSize;
    stream.next_out = target;
    stream.avail_out = targetSize;
    stream.zalloc = 0;
    stream.zfree = 0;

    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK)
        return false;

    // Original bug: Z_OK means the output buffer filled before the stream finished, yet it counts
    // as success, so data that does not fit in target is truncated silently.
    int err = deflate(&stream, Z_FINISH);
    if (err != Z_OK && err != Z_STREAM_END)
        return false;

    if (deflateEnd(&stream) != Z_OK)
        return false;

    written = stream.total_out;
    return true;
}

bool CFileSystem::zipInflateData(unsigned char* target, unsigned int targetSize,
    unsigned char* source, unsigned int sourceSize, unsigned int& written)
{
    if (!source || !target)
        return false;

    z_stream stream;
    stream.next_in = source;
    stream.avail_in = sourceSize;
    stream.next_out = target;
    stream.avail_out = targetSize;
    stream.zalloc = 0;
    stream.zfree = 0;

    if (inflateInit(&stream) != Z_OK)
        return false;

    // As in zipDeflateData, Z_OK (target full before the stream ended) counts as success.
    int err = inflate(&stream, Z_FINISH);
    if (err != Z_OK && err != Z_STREAM_END)
        return false;

    if (inflateEnd(&stream) != Z_OK)
        return false;

    written = stream.total_out;
    return true;
}

void CFileSystem::addZipAlias(const char* extension)
{
    ZipExtensions[extension] = true;
}

void CFileSystem::dropZipReaders()
{
    for (std::map<ox::core::CString<char>, CZipReader*>::iterator it = ZipReaders.begin();
        it != ZipReaders.end(); ++it)
    {
        if (it->second)
            it->second->drop();
    }

    ZipReaders.clear();
}

//! The extensions are tried in map order (byte order of the extension); the first one found after
//! the first character of the path wins, and the path is cut right after it.
ox::core::CString<char> CFileSystem::getZipInPath(const char* filename)
{
    ox::core::CString<char> path(filename);
    ox::core::CString<char> zipPath;

    for (std::map<ox::core::CString<char>, bool>::iterator it = ZipExtensions.begin();
        it != ZipExtensions.end(); ++it)
    {
        if (path.findNext(it->first.c_str(), 0) <= 0)
            continue;

        zipPath = path.subString(0, path.findNext(it->first.c_str(), 0) + it->first.size());
        break;
    }

    return ox::core::CString<char>(zipPath);
}

//! The reader is stored under filename as given; the archive itself is opened from the
//! alias-resolved path, without case or path folding.
CZipReader* CFileSystem::getZipReader(const char* filename)
{
    ox::core::CString<char> name(filename);
    CZipReader* reader = ZipReaders[name];
    ox::core::CString<char> path = intern_resolveAliases(filename);

    if (!reader)
    {
        ox::io::IReadFile* file = createReadFile(path.c_str());
        if (file)
        {
            os::Printer::log("Opening zip file", name.c_str(), ox::event::ELL_INFORMATION);
            CZipReader* zip = new CZipReader(file, false, false);
            if (zip)
                ZipReaders[name] = zip;
            else
                os::Printer::log("Error opening zip file", name.c_str(), ox::event::ELL_ERROR);
            file->drop();
        }
    }

    return ZipReaders[name];
}

//! Returns the string of the current working directory
const char* CFileSystem::getWorkingDirectory()
{
    return WorkingDirectory.c_str();
}

void CFileSystem::addDirectoryAlias(const char* alias, const char* directory)
{
    Aliases[alias] = directory;
}

const char* CFileSystem::getDirectoryFromAlias(const char* alias)
{
    return Aliases[alias].c_str();
}

ox::io::IFilePath* CFileSystem::resolveAliases(const char* filename)
{
    ox::core::CString<char> path = intern_resolveAliases(filename);
    return new CFilePath(path.c_str());
}

//! The directory is used as given (aliases unresolved). When chdir fails and the directory points
//! into an archive that getZipReader can open, the working directory becomes that path inside the
//! archive; the process directory stays where it was. The check of the folder inside the archive
//! is passed an empty string, so any path below a readable archive is accepted.
bool CFileSystem::changeWorkingDirectoryTo(const char* directory)
{
    bool changed = chdir(directory) == 0;

    if (!changed)
    {
        ox::core::CString<char> zipPath = getZipInPath(directory);
        if (zipPath.size() > 0)
        {
            CZipReader* reader = getZipReader(zipPath.c_str());
            if (reader)
            {
                // Original bug: the folder below the archive is cut from zipPath itself, not from
                // directory, so directoryExists always gets "" and accepts any path in the archive.
                ox::core::CString<char> inZip = zipPath.subStringToEnd(zipPath.size());
                if (reader->directoryExists(inZip.c_str()) == true)
                {
                    WorkingDirectoryInZip = true;
                    WorkingZip = zipPath;
                    changed = true;
                }
            }
        }
    }
    else
    {
        WorkingDirectoryInZip = false;
        WorkingZip = 0;
    }

    if (changed)
        WorkingDirectory = ox::core::CString<char>(directory);

    return changed;
}

bool CFileSystem::createDirectory(const char* directory)
{
    return mkdir(intern_resolveAliases(directory).c_str(), 0755) == 0;
}

//! Lists filter matches in directory (aliases resolved for the change of directory). The working
//! directory is changed for the listing and set back afterwards. Inside an archive the list comes
//! from the archive's folder after the archive and its '/', and mode is ignored; on disk see
//! CFileList. If the directory can be entered neither way, the current working directory is
//! listed instead.
ox::io::IFileList* CFileSystem::createFileList(const char* filter, const char* directory, ox::io::EFileList mode)
{
    ox::core::CString<char> oldDirectory(WorkingDirectory);
    changeWorkingDirectoryTo(intern_resolveAliases(directory).c_str());

    ox::io::IFileList* list;
    if (WorkingDirectoryInZip == true && WorkingZip.size() > 0)
    {
        ox::core::CString<char> inZip = WorkingDirectory.subStringToEnd(WorkingZip.size() + 1);
        list = new CZipFileList(ZipReaders[WorkingZip], filter, inZip.c_str());
    }
    else
        list = new CFileList(filter, directory, mode);

    changeWorkingDirectoryTo(oldDirectory.c_str());
    return list;
}

//! fopen(path, "rb") decides on disk, so directories count as existing. With searchArchives the
//! archive must already be open under its alias-resolved path (createFileList and
//! changeWorkingDirectoryTo open it so); the rest of the path, '/' included, is looked up with
//! CZipReader::findFile.
bool CFileSystem::existFile(const char* filename, bool searchArchives)
{
    ox::core::CString<char> path = intern_resolveAliases(filename);
    if (path.size() > 0)
    {
        FILE* file = fopen(path.c_str(), "rb");
        if (file)
        {
            fclose(file);
            return true;
        }

        if (searchArchives == true)
        {
            ox::core::CString<char> zipPath = getZipInPath(path.c_str());
            ox::core::CString<char> inZip = path.subStringToEnd(zipPath.size());
            if (zipPath.size() > 0 && ZipReaders[zipPath] != 0)
                return ZipReaders[zipPath]->findFile(inZip.c_str()) != -1;
        }
    }

    return false;
}

//! Creates a XML Reader from a file.
ox::io::IXMLReader* CFileSystem::createXMLReader(const char* filename)
{
    ox::io::IReadFile* file = createAndOpenFile(filename);
    if (!file)
        return 0;

    ox::io::IXMLReader* reader = createXMLReader(file);
    file->drop();
    return reader;
}

//! Creates a XML Writer from a file.
ox::io::IXMLWriter* CFileSystem::createXMLWriter(const char* filename)
{
    ox::io::IWriteFile* file = createAndWriteFile(filename);
    ox::io::IXMLWriter* writer = createXMLWriter(file);
    file->drop();
    return writer;
}

//! Creates a XML Writer from a file.
ox::io::IXMLWriter* CFileSystem::createXMLWriter(ox::io::IWriteFile* file)
{
    return new CXMLWriter(file);
}

bool CFileSystem::deleteFile(const char* filename)
{
    return remove(intern_resolveAliases(filename).c_str()) == 0;
}

//! Original bug: the resolved paths reach rename(3) the other way round, so newName is renamed to
//! filename. The game never calls this.
bool CFileSystem::renameFile(const char* filename, const char* newName)
{
    ox::core::CString<char> oldPath = intern_resolveAliases(filename);
    ox::core::CString<char> newPath = intern_resolveAliases(newName);
    return rename(newPath.c_str(), oldPath.c_str()) == 0;
}

//! creates a filesystem which is able to open files from the ordinary file system,
//! and out of zipfiles, which are able to be added to the filesystem.
ox::io::IFileSystem* createFileSystem()
{
    return new CFileSystem();
}

ox::io::IReadFile* CFileSystem::readFileIntoMemory(const char* filename)
{
    ox::io::IReadFile* file = createAndOpenFile(filename);
    ox::io::IReadFile* memoryFile = 0;

    if (file)
    {
        int size = file->getSize();
        char* data = new char[size];
        file->read(data, size);
        memoryFile = createMemoryReadFile(data, size, filename, true);
        if (!memoryFile)
            delete [] data;
        file->drop();
    }

    return memoryFile;
}

//! Creates a XML Reader from a file.
ox::io::IXMLReader* CFileSystem::createXMLReader(ox::io::IReadFile* file)
{
    if (!file)
        return 0;

    CTextReader* txtreader = new CTextReader(file);
    CXMLReader* xmlreader = new CXMLReader(txtreader);

    txtreader->drop();

    return xmlreader;
}

} // end namespace io
} // end namespace daisy
