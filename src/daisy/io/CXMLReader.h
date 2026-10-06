// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial and provisional: only what CFileSystem::createXMLReader needs is declared, sized as the
// Linux build allocates it. The game never uses XML.

#ifndef DAISY_IO_CXMLREADER_H
#define DAISY_IO_CXMLREADER_H

#include "ox/io/IXMLReader.h"

namespace daisy {
namespace io {

class CTextReader;

class CXMLReader : public ox::io::IXMLReader
{
public:
    CXMLReader(CTextReader* file);

private:
    char Unrecovered[0x68 - sizeof(ox::io::IXMLReader)];
};

} // end namespace io
} // end namespace daisy

#endif
