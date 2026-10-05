// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef OX_GAME_CCONFIGURATION_H
#define OX_GAME_CCONFIGURATION_H

#include "../TArray.h"
#include "../core/CString.h"

namespace ox {
namespace io { class IFileSystem; class IReadFile; class IWriteFile; }
namespace game {

class CConfigBlock;

//! A configuration file of blocks and attributes, used for settings, profiles, languages and
//! mods. Attributes are addressed as "block:attribute". The text format is
//!     # comment
//!     block
//!     {
//!         attribute = value;
//!     }
//! where block and attribute names are letters, digits and spaces, and a backslash escapes a
//! following ';', '{' or '}' in a value. Files are 16-bit characters after a 0xfeff byte order
//! mark (as write writes them) or text in the current locale.
class CConfiguration
{
public:
    CConfiguration(io::IFileSystem* fileSystem);
    virtual ~CConfiguration();

    bool read(const char* filename);
    //! Parses file and drops it.
    bool read(io::IReadFile* file);
    void write(const char* filename);
    //! Writes every block to file and drops it. Always returns false.
    bool write(io::IWriteFile* file);
    bool blockExists(const wchar_t* name);
    bool blockExists(const core::CString<wchar_t>& name);
    CConfigBlock* getBlock(const wchar_t* name);
    CConfigBlock* getBlock(const core::CString<wchar_t>& name);
    TArray<CConfigBlock*>& getBlocks();
    bool attributeExists(const wchar_t* name);
    bool attributeExists(const core::CString<wchar_t>& name);
    //! Sets value to the attribute's value, or to an empty string if there is no such attribute.
    void getAttribute(const wchar_t* name, core::CString<wchar_t>& value);
    void getAttribute(const core::CString<wchar_t>& name, core::CString<wchar_t>& value);
    //! Reads a string that setAttributeAsBase64 stored.
    void getAttributeFromBase64(const wchar_t* name, core::CString<wchar_t>& value);
    void getAttributeFromBase64(const core::CString<wchar_t>& name, core::CString<wchar_t>& value);
    int getAttributeAsInt(const wchar_t* name);
    int getAttributeAsInt(const core::CString<wchar_t>& name);
    float getAttributeAsFloat(const wchar_t* name);
    float getAttributeAsFloat(const core::CString<wchar_t>& name);
    //! Sets an attribute, adding its block if there is none yet.
    void setAttribute(const core::CString<wchar_t>& name, const core::CString<wchar_t>& value);
    void setAttribute(const wchar_t* name, const wchar_t* value);
    void setAttribute(const wchar_t* name, int value);
    void setAttribute(const wchar_t* name, float value);
    //! Stores value as base64url of its 16-bit characters and terminator.
    void setAttributeAsBase64(const core::CString<wchar_t>& name, const core::CString<wchar_t>& value);

private:
    bool parseConfigFile(const wchar_t* text);
    //! Whether ch follows in text after whitespace and comments.
    bool isNextChar(const wchar_t* text, wchar_t ch);
    //! Parses a name (identifier) or a value up to one of delimiters into result; returns the
    //! number of characters read including the delimiter, or -1 on an error.
    int parseString(const wchar_t* text, core::CString<wchar_t>& result,
        const core::CString<wchar_t>& delimiters, bool identifier);
    void printError(const wchar_t* text);
    //! Parses "name = value;" into block; returns the number of characters read or -1.
    int parseAttribute(const wchar_t* text, CConfigBlock* block);
    //! The number of characters up to and including the first of delimiters, or -1.
    int discardString(const wchar_t* text, const core::CString<wchar_t>& delimiters);
    //! Splits "block:attribute" and returns the block, or 0.
    CConfigBlock* getBlockEx(const core::CString<wchar_t>& name, core::CString<wchar_t>& blockName,
        core::CString<wchar_t>& attributeName);

    io::IFileSystem* FileSystem;
    TArray<CConfigBlock*> Blocks;
};

} // end namespace game
} // end namespace ox

#endif
