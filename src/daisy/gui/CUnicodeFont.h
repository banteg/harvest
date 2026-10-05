// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_GUI_CUNICODEFONT_H
#define DAISY_GUI_CUNICODEFONT_H

#include <vector>
#include "ox/gui/IGUIFont.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"

namespace daisy {
namespace gui {

//! A character of a unicode font: one sprite animation of the font's package, named by its code.
struct SUnicodeChar
{
    wchar_t Character;
    ox::core::CDimension2d<int> Size;
    //! Created on first use, except for the first character, whose height sets the font's.
    ox::video::ISpriteAnimationState* Animation;
};

//! A font whose characters are the animations of a sprite package, named by their decimal character
//! codes. "#0" to "#9" in a text switch to one of ten colors and "#o" back to the original color.
class CUnicodeFont : public ox::gui::IGUIFont
{
public:
    CUnicodeFont(ox::video::IVideoDriver* driver);
    virtual ~CUnicodeFont();

    //! Loads the sprite package of the characters.
    bool load(const char* filename);

    virtual void draw(const wchar_t* text, const ox::core::CRect<int>& position, ox::video::SColor color,
        ox::gui::EFontHorizontalAlign horizontal, ox::gui::EFontVerticalAlign vertical,
        const ox::core::CRect<int>* clip);
    virtual ox::core::CDimension2d<int> getDimension(const wchar_t* text);
    virtual int getCharacterFromPos(const wchar_t* text, int x);
    virtual void setOriginalColor(const ox::video::SColor& color);
    virtual ox::video::SColor getRecentColor();

private:
    //! The character's entry, or 0 when the package has no such character.
    SUnicodeChar* findCharacter(wchar_t character);
    //! Creates the character's animation and reads its size.
    void loadCharacter(SUnicodeChar* character);

    ox::video::IVideoDriver* Driver;
    ox::video::ISpritePackage* Package;
    //! Hash buckets of the characters by code.
    std::vector<SUnicodeChar>* Characters;
    int Height;
    //! Advance of characters the package lacks: half the height.
    int MissingCharacterWidth;
    ox::video::SColor OriginalColor;
    //! The color the last draw ended with.
    ox::video::SColor RecentColor;
};

} // end namespace gui
} // end namespace daisy

#endif
