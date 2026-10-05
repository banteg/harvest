// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIFont.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIFONT_H
#define DAISY_GUI_CGUIFONT_H

#include <vector>
#include "ox/gui/IGUIFont.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/ITexture.h"

namespace daisy {
namespace gui {

//! A bitmap font read from a texture whose corner pixels mark each character's rectangle.
class CGUIFont : public ox::gui::IGUIFont
{
public:
    CGUIFont(ox::video::IVideoDriver* driver);
    virtual ~CGUIFont();

    //! loads a font file
    bool load(const char* filename);

    //! loads a font file
    bool load(ox::io::IReadFile* file);

    //! draws a text and clips it to the specified rectangle if wanted
    virtual void draw(const wchar_t* text, const ox::core::CRect<int>& position, ox::video::SColor color,
        ox::gui::EFontHorizontalAlign horizontal, ox::gui::EFontVerticalAlign vertical,
        const ox::core::CRect<int>* clip);

    //! returns the dimension of a text
    virtual ox::core::CDimension2d<int> getDimension(const wchar_t* text);

    //! Calculates the index of the character in the text which is on a specific position.
    virtual int getCharacterFromPos(const wchar_t* text, int x);

    //! The characters keep the texture's colors, so these do nothing.
    virtual void setOriginalColor(const ox::video::SColor& color) {}
    virtual ox::video::SColor getRecentColor() { return ox::video::SColor(255, 255, 255, 255); }

private:
    //! load & prepare font from ITexture
    bool loadTexture(ox::video::ITexture* texture);

    void readPositions16bit(ox::video::ITexture* texture, int& lowerRightPositions);
    void readPositions32bit(ox::video::ITexture* texture, int& lowerRightPositions);

    inline int getWidthFromCharacter(wchar_t c);

    ox::video::IVideoDriver* Driver;
    std::vector<ox::core::CRect<int> > Positions;
    ox::video::ITexture* Texture;
    unsigned int WrongCharacter;
};

} // end namespace gui
} // end namespace daisy

#endif
