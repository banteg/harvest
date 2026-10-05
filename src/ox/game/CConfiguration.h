// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef OX_GAME_CCONFIGURATION_H
#define OX_GAME_CCONFIGURATION_H

#include "../TArray.h"
#include "../core/CString.h"

namespace ox {
namespace io { class IFileSystem; class IReadFile; }
namespace game {

class CConfigBlock;

//! A configuration file of blocks and attributes, such as "mod:name".
class CConfiguration
{
public:
    CConfiguration(io::IFileSystem* fileSystem);
    virtual ~CConfiguration();

    bool read(const char* filename);
    bool read(io::IReadFile* file);
    void write(const char* filename);
    bool attributeExists(const wchar_t* name);
    bool getAttribute(const wchar_t* name, core::CString<wchar_t>& value);
    bool getAttributeFromBase64(const wchar_t* name, core::CString<wchar_t>& value);
    int getAttributeAsInt(const wchar_t* name);
    float getAttributeAsFloat(const wchar_t* name);
    void setAttribute(const core::CString<wchar_t>& name, const core::CString<wchar_t>& value);
    void setAttribute(const wchar_t* name, const wchar_t* value);
    void setAttribute(const wchar_t* name, int value);
    void setAttribute(const wchar_t* name, float value);
    void setAttributeAsBase64(const core::CString<wchar_t>& name, const core::CString<wchar_t>& value);
    TArray<CConfigBlock*>& getBlocks();
    CConfigBlock* getBlock(const core::CString<wchar_t>& name);

private:
    io::IFileSystem* FileSystem;
    TArray<CConfigBlock*> Blocks;
};

} // end namespace game
} // end namespace ox

#endif
