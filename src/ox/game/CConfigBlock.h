// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only the methods recovered code calls are declared; the attribute storage is not recovered yet.

#ifndef OX_GAME_CCONFIGBLOCK_H
#define OX_GAME_CCONFIGBLOCK_H

#include "../core/CString.h"

namespace ox {
namespace game {

//! A named block of attributes in a configuration file.
class CConfigBlock
{
public:
    CConfigBlock(const wchar_t* name);
    virtual ~CConfigBlock();

    bool attributeExists(const wchar_t* name);
    const core::CString<wchar_t>& getBlockName();
    bool getAttribute(const wchar_t* name, core::CString<wchar_t>& value);
    int getAttributeAsInt(const wchar_t* name);
    float getAttributeAsFloat(const wchar_t* name);

private:
    core::CString<wchar_t> Name;
};

} // end namespace game
} // end namespace ox

#endif
