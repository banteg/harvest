// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial and provisional: only what CFileSystem::createXMLReader needs is declared, sized as the
// Linux build allocates it. The game never uses XML.

#ifndef DAISY_IO_CTEXTREADER_H
#define DAISY_IO_CTEXTREADER_H

#include "ox/io/IReadFile.h"

namespace daisy {
namespace io {

//! Reads a whole text file and converts it to wide characters for the XML reader.
class CTextReader : public ox::IUnknown
{
public:
    CTextReader(ox::io::IReadFile* file);

private:
    char Unrecovered[0x30 - sizeof(ox::IUnknown)];
};

} // end namespace io
} // end namespace daisy

#endif
