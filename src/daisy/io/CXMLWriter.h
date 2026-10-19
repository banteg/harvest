// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial and provisional: only what CFileSystem::createXMLWriter needs is declared, sized as the
// Linux build allocates it. The game never uses XML.

#ifndef DAISY_IO_CXMLWRITER_H
#define DAISY_IO_CXMLWRITER_H

#include "ox/io/IXMLWriter.h"
#include "ox/io/IWriteFile.h"

namespace ox { namespace io {} }
namespace daisy {
namespace io {
using namespace ox::io;

#ifdef HARVEST_PORT
class CXMLWriter : public ox::io::IXMLWriter
{
public:
    CXMLWriter(ox::io::IWriteFile* file);

private:
    char Unrecovered[0x20 - sizeof(ox::io::IXMLWriter)];
};
#else
class CXMLWriter : public IXMLWriter
    {
    public:

        //! Constructor
        CXMLWriter(IWriteFile* file);

        //! Destructor
        virtual ~CXMLWriter();

        //! Writes a xml 1.0 header like <?xml version="1.0"?>
        virtual void writeXMLHeader();

        //! Writes an xml element with maximal 5 attributes
        virtual void writeElement(const wchar_t* name, bool empty=false,
            const wchar_t* attr1Name = 0, const wchar_t* attr1Value = 0,
            const wchar_t* attr2Name = 0, const wchar_t* attr2Value = 0,
            const wchar_t* attr3Name = 0, const wchar_t* attr3Value = 0,
            const wchar_t* attr4Name = 0, const wchar_t* attr4Value = 0,
            const wchar_t* attr5Name = 0, const wchar_t* attr5Value = 0);

        //! Writes a comment into the xml file
        virtual void writeComment(const wchar_t* comment);

        //! Writes the closing tag for an element. Like </foo>
        virtual void writeClosingTag(const wchar_t* name);

        //! Writes a text into the file. All occurrences of special characters like
        //! & (&amp;), < (&lt;), > (&gt;), and " (&quot;) are automaticly replaced.
        virtual void writeText(const wchar_t* text);

        //! Writes a line break
        virtual void writeLineBreak();

        struct XMLSpecialCharacters
        {
            wchar_t Character;
            wchar_t* Symbol;
        };

    private:

        void writeAttribute(const wchar_t* att, const wchar_t* name);

        IWriteFile* File;
    };
#endif

} // end namespace io
} // end namespace daisy

#endif
