// The XML reader and writer CFileSystem can create (createXMLReader, createXMLWriter). The game
// never uses XML, so the port only provides constructors for the recovered, sized-only classes;
// the objects do nothing.

#include "daisy/io/CTextReader.h"
#include "daisy/io/CXMLReader.h"
#include "daisy/io/CXMLWriter.h"

namespace daisy {
namespace io {

CTextReader::CTextReader(ox::io::IReadFile*)
{
}

CXMLReader::CXMLReader(CTextReader*)
{
}

CXMLWriter::CXMLWriter(ox::io::IWriteFile*)
{
}

} // end namespace io
} // end namespace daisy
