// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial and provisional: only what CFileSystem::createXMLWriter needs is declared, sized as the
// Linux build allocates it. The game never uses XML.

#ifndef DAISY_IO_CXMLWRITER_H
#define DAISY_IO_CXMLWRITER_H

#include "ox/io/IXMLWriter.h"
#include "ox/io/IWriteFile.h"

namespace daisy {
namespace io {

class CXMLWriter : public ox::io::IXMLWriter
{
public:
    CXMLWriter(ox::io::IWriteFile* file);

private:
    char Unrecovered[0x20 - sizeof(ox::io::IXMLWriter)];
};

} // end namespace io
} // end namespace daisy

#endif
