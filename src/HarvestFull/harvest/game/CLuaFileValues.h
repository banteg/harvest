// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only the layout and helpers used by creative-building saves are recovered.

#ifndef HARVEST_GAME_CLUAFILEVALUES_H
#define HARVEST_GAME_CLUAFILEVALUES_H

#include "ox/TArray.h"
#include "ox/core/CString.h"
#include "lua.hpp"

namespace ox { namespace io { class IReadFile; class IWriteFile; } }
namespace harvest {
namespace game {

//! A Lua key or value, represented as either a string or a number.
//! Member names are ours; the class name is from the Mac symbols.
struct SLuaAttribute
{
    ox::core::CString<char> String;
    float Number;
    bool IsString;
};

struct SLuaFilePair
{
    SLuaAttribute Key;
    SLuaAttribute Value;
};

class CLuaFileValues
{
public:
    static void addLuaTableVars(lua_State* L, const ox::core::CString<char>& prefix,
        ox::TArray<SLuaFilePair>& values);
    static void getLuaTableVars(lua_State* L, int table, const ox::core::CString<char>& prefix,
        ox::TArray<SLuaFilePair>& values);
    static void writeLuaFilePair(ox::io::IWriteFile* file, SLuaFilePair& pair);
    static void readLuaFilePair(ox::io::IReadFile* file, SLuaFilePair& pair, int version);
    static void readLuaAttribute(ox::io::IReadFile* file, SLuaAttribute& attribute, int version);
    static void writeLuaAttribute(ox::io::IWriteFile* file, SLuaAttribute& attribute);
};

} // end namespace game
} // end namespace harvest

#endif
