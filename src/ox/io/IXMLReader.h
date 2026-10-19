// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial and provisional: the game never uses XML, so only the class is declared.

#ifndef OX_IO_IXMLREADER_H
#define OX_IO_IXMLREADER_H

#include "../IUnknown.h"

namespace ox {
namespace io {

enum EXML_NODE
{
    //! No xml node. This is usually the node if you did not read anything yet.
    EXN_NONE,

    //! A xml element, like <foo>
    EXN_ELEMENT,

    //! End of an xml element, like </foo>
    EXN_ELEMENT_END,

    //! Text within a xml element: <foo> this is the text. </foo>
    EXN_TEXT,

    //! An xml comment like <!-- I am a comment --> or a DTD definition.
    EXN_COMMENT,

    //! Unknown element.
    EXN_UNKNOWN
};

class IXMLReader : public ox::IUnknown
    {
    public:

        //! Destructor
        virtual ~IXMLReader() {};

        //! Reads forward to the next xml node.
        //! \return Returns false, if there was no further node.
        virtual bool read() = 0;

        //! Returns the type of the current XML node.
        virtual EXML_NODE getNodeType() = 0;

        //! Returns attribute count of the current XML node. This is usually
        //! non null if the current node is EXN_ELEMENT, and the element has attributes.
        //! \return Returns amount of attributes of this xml node.
        virtual int getAttributeCount() = 0;

        //! Returns name of an attribute.
        //! \param idx: Zero based index, should be something between 0 and getAttributeCount()-1.
        //! \return Name of the attribute, 0 if an attribute with this index does not exist.
        virtual const wchar_t* getAttributeName(int idx) = 0;

        //! Returns the value of an attribute.
        //! \param idx: Zero based index, should be something between 0 and getAttributeCount()-1.
        //! \return Value of the attribute, 0 if an attribute with this index does not exist.
        virtual const wchar_t* getAttributeValue(int idx) = 0;

        //! Returns the value of an attribute.
        //! \param name: Name of the attribute.
        //! \return Value of the attribute, 0 if an attribute with this name does not exist.
        virtual const wchar_t* getAttributeValue(const wchar_t* name) = 0;

        //! Returns the value of an attribute, like getAttributeValue(), but does not
        //! return 0 if the attribute does not exist. An empty string ("") is returned then.
        //! \param name: Name of the attribute.
        //! \return Value of the attribute, and "" if an attribute with this name does not exist
        virtual const wchar_t* getAttributeValueSafe(const wchar_t* name) = 0;

        //! Returns the value of an attribute as integer.
        //! \param name: Name of the attribute.
        //! \return Value of the attribute as integer, and 0 if an attribute with this name does not exist or
        //! the value could not be interpreted as integer.
        virtual int getAttributeValueAsInt(const wchar_t* name) = 0;

        //! Returns the value of an attribute as float.
        //! \param name: Name of the attribute.
        //! \return Value of the attribute as float, and 0 if an attribute with this name does not exist or
        //! the value could not be interpreted as float.
        virtual float getAttributeValueAsFloat(const wchar_t* name) = 0;

        //! Returns the name of the current node. Only non null, if the node type is
        //! EXN_ELEMENT.
        //! \return Name of the current node or 0 if the node has no name.
        virtual const wchar_t* getNodeName() = 0;

        //! Returns data of the current node. Only non null if the node has some
        //! data and it is of type EXN_TEXT or EXN_UNKNOWN.
        virtual const wchar_t* getNodeData() = 0;

        //! Returns if an element is an empty element, like <foo />
        virtual bool isEmptyElement() = 0;
    };

} // end namespace io
} // end namespace ox

#endif
