// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef OX_GAME_CTEXTLOCALIZATION_H
#define OX_GAME_CTEXTLOCALIZATION_H

#include "CConfiguration.h"

namespace ox {
namespace game {

//! A language file: a configuration whose attributes are the localized texts.
class CTextLocalization : public CConfiguration
{
public:
    CTextLocalization(io::IFileSystem* fileSystem)
        : CConfiguration(fileSystem)
    {
    }

    virtual ~CTextLocalization() {}

    //! The text of a key, with escaped line breaks resolved.
    core::CString<wchar_t> getText(const wchar_t* key);
};

} // end namespace game
} // end namespace ox

#endif
