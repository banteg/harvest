// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CConfigBlock.h"
#include <cwchar>
#include "../core/CStringConversions.h"
#include "../io/CHelpIO.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace game {

CConfigBlock::CConfigBlock(const wchar_t* name)
{
    Name = name;
}

CConfigBlock::~CConfigBlock()
{
    int count = Attributes.size();
    for (int i = 0; i < count; ++i)
    {
        SAttribute* attribute = Attributes[i];
        delete attribute->first;
        delete attribute->second;
        delete attribute;
    }
}

bool CConfigBlock::attributeExists(const wchar_t* name)
{
    return attributeExists(core::CString<wchar_t>(name));
}

bool CConfigBlock::attributeExists(const core::CString<wchar_t>& name)
{
    return findAttribute(name) != 0;
}

CConfigBlock::SAttribute* CConfigBlock::findAttribute(const core::CString<wchar_t>& name)
{
    int count = Attributes.size();
    for (int i = 0; i < count; ++i)
    {
        SAttribute* attribute = Attributes[i];
        if (*attribute->first == name)
            return attribute;
    }
    return 0;
}

const core::CString<wchar_t>& CConfigBlock::getBlockName()
{
    return Name;
}

void CConfigBlock::getAttribute(const wchar_t* name, core::CString<wchar_t>& value)
{
    getAttribute(core::CString<wchar_t>(name), value);
}

void CConfigBlock::getAttribute(const core::CString<wchar_t>& name, core::CString<wchar_t>& value)
{
    SAttribute* attribute = findAttribute(name);
    if (!attribute)
        value = "";
    else
        value = *attribute->second;
}

int CConfigBlock::getAttributeAsInt(const wchar_t* name)
{
    return getAttributeAsInt(core::CString<wchar_t>(name));
}

int CConfigBlock::getAttributeAsInt(const core::CString<wchar_t>& name)
{
    SAttribute* attribute = findAttribute(name);
    if (!attribute)
        return -1;
    return wcstol(attribute->second->c_str(), 0, 10);
}

float CConfigBlock::getAttributeAsFloat(const wchar_t* name)
{
    return getAttributeAsFloat(core::CString<wchar_t>(name));
}

float CConfigBlock::getAttributeAsFloat(const core::CString<wchar_t>& name)
{
    SAttribute* attribute = findAttribute(name);
    if (!attribute)
        return 0;
    return core::CStringFunctions::wideToFloat(attribute->second->c_str());
}

void CConfigBlock::setAttribute(const wchar_t* name, const wchar_t* value)
{
    setAttribute(core::CString<wchar_t>(name), core::CString<wchar_t>(value));
}

void CConfigBlock::setAttribute(const core::CString<wchar_t>& name, const core::CString<wchar_t>& value)
{
    SAttribute* attribute = findAttribute(name);
    if (!attribute)
    {
        attribute = new SAttribute();
        attribute->first = new core::CString<wchar_t>(name);
        attribute->second = new core::CString<wchar_t>();
        Attributes.push_back(attribute);
    }
    *attribute->second = value;
}

void CConfigBlock::setAttribute(const wchar_t* name, int value)
{
    setAttribute(core::CString<wchar_t>(name), core::CString<wchar_t>(value));
}

void CConfigBlock::setAttribute(const wchar_t* name, float value)
{
    setAttribute(core::CString<wchar_t>(name), core::CStringFunctions::floatToWide(value));
}

void CConfigBlock::printBlock(io::IWriteFile* file)
{
    core::CString<wchar_t> text;
    text += Name;
    text += L"\n{\n";
    int count = Attributes.size();
    for (int i = 0; i < count; ++i)
    {
        SAttribute* attribute = Attributes[i];
        text += L"\t";
        text += attribute->first->c_str();
        text += L" = ";
        text += attribute->second->c_str();
        text += L";\n";
    }
    text += L"}\n";
    io::CHelpIO::writeWideString(file, text, false);
}

} // end namespace game
} // end namespace ox
