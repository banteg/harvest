// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial and provisional: only what CFileSystem::createXMLReader needs is declared, sized as the
// Linux build allocates it. The game never uses XML.

#ifndef DAISY_IO_CXMLREADER_H
#define DAISY_IO_CXMLREADER_H

#include "ox/io/IXMLReader.h"
#include "ox/core/CString.h"
#include "ox/TArray.h"

namespace ox { namespace io {} }
namespace daisy {
namespace io {
using namespace ox::io;

class CTextReader;

class CXMLReader : public IXMLReader
    {
    public:

        CXMLReader(CTextReader* reader);

        //! Destructor
        virtual ~CXMLReader();

        //! Reads forward to the next xml node. 
        //! \return Returns false, if there was no further node. 
        virtual bool read();

        //! Returns the type of the current XML node.
        virtual EXML_NODE getNodeType();

        //! Returns attribute count of the current XML node.
        virtual int getAttributeCount();

        //! Returns name of an attribute.
        virtual const wchar_t* getAttributeName(int idx);

        //! Returns the value of an attribute. 
        virtual const wchar_t* getAttributeValue(int idx);

        //! Returns the value of an attribute. 
        virtual const wchar_t* getAttributeValue(const wchar_t* name);

        //! Returns the value of an attribute
        virtual const wchar_t* getAttributeValueSafe(const wchar_t* name);

        //! Returns the value of an attribute as integer. 
        virtual int getAttributeValueAsInt(const wchar_t* name);

        //! Returns the value of an attribute as float. 
        virtual float getAttributeValueAsFloat(const wchar_t* name);

        //! Returns the name of the current node.
        virtual const wchar_t* getNodeName();

        //! Returns data of the current node.
        virtual const wchar_t* getNodeData();

        //! Returns if an element is an empty element, like <foo />
        virtual bool isEmptyElement();

        struct XMLSpecialCharacters
        {
            wchar_t Character;
            wchar_t* Symbol;
        };

    private:

        void parseCurrentNode();

        //! sets the state that text was found
        void setText(wchar_t* start, wchar_t* end);

        //! ignores an xml definition like <?xml something />
        void ignoreDefinition();

        //! parses a comment
        void parseComment();

        //! parses an opening xml element and reads attributes
        void parseOpeningXMLElement();

        //! parses an closing xml tag
        void parseClosingXMLElement();

        inline bool isWhiteSpace(wchar_t c)
        {
            return (c==L' ' || c==L'\t' || c==L'\n' || c==L'\r');
        }

        CTextReader* Reader;
        wchar_t* P;
        wchar_t* TextBegin;
        unsigned int TextSize;

        EXML_NODE CurrentNodeType;
        ox::core::CString<wchar_t> NodeName;
        bool IsEmptyElement;

        struct SAttribute
        {
            ox::core::CString<wchar_t> Name;
            ox::core::CString<wchar_t> Value;
        };

        SAttribute* getAttributeByName(const wchar_t* name);
        ox::core::CString<wchar_t> replaceSpecialCharacters(ox::core::CString<wchar_t>& str);

        ox::TArray<SAttribute> Attributes;
    };

} // end namespace io
} // end namespace daisy

#endif
