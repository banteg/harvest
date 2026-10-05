// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CConfiguration.h"
#include <cwchar>
#include "CConfigBlock.h"
#include "../algo/CBase64url.h"
#include "../core/CStringFunctions.h"
#include "../io/CHelpIO.h"
#include "../io/CMemReadFile.h"
#include "../io/CMemWriteFile.h"
#include "../io/IFileSystem.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace game {

CConfiguration::CConfiguration(io::IFileSystem* fileSystem)
{
    FileSystem = fileSystem;
}

CConfiguration::~CConfiguration()
{
    int count = Blocks.size();
    for (int i = 0; i < count; ++i)
        delete Blocks[i];
}

bool CConfiguration::read(const char* filename)
{
    io::IReadFile* file = FileSystem->createAndOpenFile(filename);
    if (file)
        return read(file);
    return false;
}

bool CConfiguration::read(io::IReadFile* file)
{
    if (!file)
        return false;

    int length = file->getSize() / 2;
    if (length == 0)
    {
        file->drop();
        return false;
    }

    core::CString<wchar_t> text;
    if (io::CHelpIO::readUShort(file) != 0xfeff)
    {
        // no byte order mark: text in the current locale
        core::CString<char> ansi;
        file->seek(-2, true);
        io::CHelpIO::readString(file, ansi);
        text = core::CStringFunctions::ansiToWide(ansi);
    }
    else
    {
        io::CHelpIO::readWideString(file, text);
        if (text.size() == 0)
        {
            file->drop();
            return false;
        }
    }

    bool result = parseConfigFile(text.c_str());
    file->drop();
    return result;
}

bool CConfiguration::parseConfigFile(const wchar_t* text)
{
    if (!text)
        return false;

    core::CString<wchar_t> name;
    core::CString<wchar_t> blockStart = L"{";
    const wchar_t* position = text;
    while (!isNextChar(position, 0))
    {
        int length = parseString(position, name, blockStart, true);
        if (length == -1)
        {
            printError(position);
            return false;
        }
        position += length;

        CConfigBlock* block = new CConfigBlock(name.c_str());
        Blocks.push_back(block);
        while (!isNextChar(position, L'}'))
        {
            length = parseAttribute(position, block);
            if (length == -1)
            {
                core::CStringFunctions::wideToAnsi(core::CString<wchar_t>(position));
                return false;
            }
            position += length;
        }
        position += discardString(position, L"}");
    }
    return true;
}

bool CConfiguration::isNextChar(const wchar_t* text, wchar_t ch)
{
    int i = 0;
    while (text[i] != ch)
    {
        if (text[i] == L'#')
        {
            int length = discardString(&text[i], L"\n\r");
            if (length == -1)
                return false;
            i += length;
        }
        else if (text[i] != L'\t' && text[i] != L' ' && text[i] != L'\n' && text[i] != L'\r')
            return false;
        ++i;
    }
    return true;
}

int CConfiguration::parseString(const wchar_t* text, core::CString<wchar_t>& result,
    const core::CString<wchar_t>& delimiters, bool identifier)
{
    core::CString<wchar_t> reserved = L";{}";
    core::CString<wchar_t> whitespace = L"\n\r\t";
    result = L"";
    bool escaped = false;
    int start = -1;
    unsigned int i = 0;
    int end = 0;
    while (true)
    {
        wchar_t c = text[i];
        if (!escaped && delimiters.findFirst(c) != -1)
            break;
        if (c == 0)
            return -1;

        if (c == L'\n' || c == L'\r' || c == L'\t')
            escaped = false;
        else if (c == L'\\' && delimiters.findFirst(text[i + 1]) != -1)
            escaped = true;
        else if (c == L'#' && identifier)
        {
            // a comment up to the end of the line
            int length = discardString(&text[i], L"\n\r");
            if (length == -1)
                return -1;
            i += length;
            escaped = false;
        }
        else if (!identifier)
        {
            result.append(c);
            // spaces before the string are skipped
            if (start == -1 && c != L' ')
                start = result.size() - 1;
            end = result.size();
            escaped = false;
        }
        else if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9'))
        {
            result.append(c);
            // spaces before the string are skipped
            if (start == -1 && c != L' ')
                start = result.size() - 1;
            end = result.size();
            escaped = false;
        }
        else if (c == L' ')
            escaped = false;
        else
            return -1;
        ++i;
    }

    if (start == -1)
        result = "";
    else
        result = result.subString(start, end - start);
    // names cannot contain spaces
    if (identifier && result.findFirst(L' ') != -1)
        return -1;
    return i + 1;
}

void CConfiguration::printError(const wchar_t* text)
{
    core::CStringFunctions::wideToAnsi(core::CString<wchar_t>(text));
}

int CConfiguration::parseAttribute(const wchar_t* text, CConfigBlock* block)
{
    core::CString<wchar_t> nameEnd = L"=";
    core::CString<wchar_t> valueEnd = L";";
    core::CString<wchar_t> name;
    core::CString<wchar_t> value;
    int nameLength = parseString(text, name, nameEnd, true);
    if (nameLength == -1)
        return -1;
    int valueLength = parseString(&text[nameLength], value, valueEnd, false);
    if (valueLength == -1)
        return -1;
    block->setAttribute(name, value);
    return nameLength + valueLength;
}

int CConfiguration::discardString(const wchar_t* text, const core::CString<wchar_t>& delimiters)
{
    int i = 0;
    do
    {
        if (delimiters.findFirst(text[i++]) != -1)
            return i;
    } while (text[i]);
    return -1;
}

void CConfiguration::write(const char* filename)
{
    io::IWriteFile* file = FileSystem->createAndWriteFile(filename, false);
    if (file)
        write(file);
}

bool CConfiguration::write(io::IWriteFile* file)
{
    if (file)
    {
        io::CHelpIO::writeUShort(file, 0xfeff);
        int count = Blocks.size();
        for (int i = 0; i < count; ++i)
            Blocks[i]->printBlock(file);
        file->drop();
    }
    return false;
}

bool CConfiguration::blockExists(const wchar_t* name)
{
    return blockExists(core::CString<wchar_t>(name));
}

bool CConfiguration::blockExists(const core::CString<wchar_t>& name)
{
    return getBlock(name) != 0;
}

CConfigBlock* CConfiguration::getBlock(const core::CString<wchar_t>& name)
{
    int count = Blocks.size();
    for (int i = 0; i < count; ++i)
    {
        CConfigBlock* block = Blocks[i];
        if (block->Name == name)
            return block;
    }
    return 0;
}

bool CConfiguration::attributeExists(const wchar_t* name)
{
    return attributeExists(core::CString<wchar_t>(name));
}

bool CConfiguration::attributeExists(const core::CString<wchar_t>& name)
{
    core::CString<wchar_t> blockName;
    core::CString<wchar_t> attributeName;
    CConfigBlock* block = getBlockEx(name, blockName, attributeName);
    if (block)
        return block->attributeExists(attributeName);
    return false;
}

TArray<CConfigBlock*>& CConfiguration::getBlocks()
{
    return Blocks;
}

CConfigBlock* CConfiguration::getBlockEx(const core::CString<wchar_t>& name,
    core::CString<wchar_t>& blockName, core::CString<wchar_t>& attributeName)
{
    int separator = name.findNext(L":", 0);
    if (separator == -1)
        return 0;
    blockName = name.subString(0, separator);
    attributeName = name.subStringToEnd(separator + 1);
    return getBlock(blockName);
}

CConfigBlock* CConfiguration::getBlock(const wchar_t* name)
{
    return getBlock(core::CString<wchar_t>(name));
}

void CConfiguration::getAttribute(const wchar_t* name, core::CString<wchar_t>& value)
{
    getAttribute(core::CString<wchar_t>(name), value);
}

void CConfiguration::getAttribute(const core::CString<wchar_t>& name, core::CString<wchar_t>& value)
{
    core::CString<wchar_t> blockName;
    core::CString<wchar_t> attributeName;
    CConfigBlock* block = getBlockEx(name, blockName, attributeName);
    value = "";
    if (block)
        block->getAttribute(attributeName, value);
}

void CConfiguration::getAttributeFromBase64(const wchar_t* name, core::CString<wchar_t>& value)
{
    getAttributeFromBase64(core::CString<wchar_t>(name), value);
}

void CConfiguration::getAttributeFromBase64(const core::CString<wchar_t>& name,
    core::CString<wchar_t>& value)
{
    getAttribute(name, value);
    core::CString<char> encoded = value.c_str();
    value = L"";
    io::CMemWriteFile* decoded = new io::CMemWriteFile();
    algo::CBase64url::decode(decoded, encoded);
    if (decoded->getSize() > 0)
    {
        io::CMemReadFile* file = new io::CMemReadFile(decoded->getData(), decoded->getSize(), false);
        io::CHelpIO::readWideString(file, value);
        delete file;
    }
    delete decoded;
}

int CConfiguration::getAttributeAsInt(const wchar_t* name)
{
    return getAttributeAsInt(core::CString<wchar_t>(name));
}

int CConfiguration::getAttributeAsInt(const core::CString<wchar_t>& name)
{
    core::CString<wchar_t> value;
    getAttribute(name, value);
    return wcstol(value.c_str(), 0, 10);
}

float CConfiguration::getAttributeAsFloat(const wchar_t* name)
{
    return getAttributeAsFloat(core::CString<wchar_t>(name));
}

float CConfiguration::getAttributeAsFloat(const core::CString<wchar_t>& name)
{
    core::CString<wchar_t> value;
    getAttribute(name, value);
    return core::CStringFunctions::wideToFloat(value.c_str());
}

void CConfiguration::setAttribute(const core::CString<wchar_t>& name, const core::CString<wchar_t>& value)
{
    core::CString<wchar_t> blockName;
    core::CString<wchar_t> attributeName;
    CConfigBlock* block = getBlockEx(name, blockName, attributeName);
    if (!block && blockName.size() > 0)
    {
        block = new CConfigBlock(blockName.c_str());
        Blocks.push_back(block);
    }
    if (block)
        block->setAttribute(attributeName, value);
}

void CConfiguration::setAttribute(const wchar_t* name, const wchar_t* value)
{
    setAttribute(core::CString<wchar_t>(name), core::CString<wchar_t>(value));
}

void CConfiguration::setAttribute(const wchar_t* name, int value)
{
    setAttribute(core::CString<wchar_t>(name), core::CString<wchar_t>(value));
}

void CConfiguration::setAttribute(const wchar_t* name, float value)
{
    setAttribute(core::CString<wchar_t>(name), core::CStringFunctions::floatToWide(value));
}

void CConfiguration::setAttributeAsBase64(const core::CString<wchar_t>& name,
    const core::CString<wchar_t>& value)
{
    io::CMemWriteFile* file = new io::CMemWriteFile();
    io::CHelpIO::writeWideString(file, value, true);
    core::CString<char> encoded;
    io::CMemReadFile* data = new io::CMemReadFile(file->getData(), file->getSize(), false);
    algo::CBase64url::encode(encoded, data);
    setAttribute(name, core::CString<wchar_t>(encoded.c_str()));
    delete data;
    delete file;
}

} // end namespace game
} // end namespace ox
