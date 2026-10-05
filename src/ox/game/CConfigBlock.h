// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef OX_GAME_CCONFIGBLOCK_H
#define OX_GAME_CCONFIGBLOCK_H

#include <utility>
#include "../TArray.h"
#include "../core/CString.h"

namespace ox {
namespace io { class IWriteFile; }
namespace game {

//! A named block of attributes in a configuration file:
//!     name
//!     {
//!         attribute = value;
//!     }
class CConfigBlock
{
public:
    CConfigBlock(const wchar_t* name);
    virtual ~CConfigBlock();

    bool attributeExists(const wchar_t* name);
    bool attributeExists(const core::CString<wchar_t>& name);
    const core::CString<wchar_t>& getBlockName();
    //! Sets value to the attribute's value, or to an empty string if there is no such attribute.
    void getAttribute(const wchar_t* name, core::CString<wchar_t>& value);
    void getAttribute(const core::CString<wchar_t>& name, core::CString<wchar_t>& value);
    //! The attribute's value in decimal, or -1 if there is no such attribute.
    int getAttributeAsInt(const wchar_t* name);
    int getAttributeAsInt(const core::CString<wchar_t>& name);
    //! The attribute's value, or 0 if there is no such attribute.
    float getAttributeAsFloat(const wchar_t* name);
    float getAttributeAsFloat(const core::CString<wchar_t>& name);
    //! Sets an attribute, adding it after the existing ones if it is new.
    void setAttribute(const wchar_t* name, const wchar_t* value);
    void setAttribute(const core::CString<wchar_t>& name, const core::CString<wchar_t>& value);
    void setAttribute(const wchar_t* name, int value);
    void setAttribute(const wchar_t* name, float value);
    //! Writes the block in the text format that CConfiguration reads.
    void printBlock(io::IWriteFile* file);

private:
    // CConfiguration compares block names directly.
    friend class CConfiguration;

    typedef std::pair<core::CString<wchar_t>*, core::CString<wchar_t>*> SAttribute;

    SAttribute* findAttribute(const core::CString<wchar_t>& name);

    core::CString<wchar_t> Name;
    TArray<SAttribute*> Attributes;
};

} // end namespace game
} // end namespace ox

#endif
