// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIFont.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye keeps the character pixels' colors, adds right and bottom alignment and stores the
// rectangles in a std::vector.

#include "CGUIFont.h"
#include "daisy/os.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! The number of character rectangles reserved up front.
static const int RESERVED_CHARACTERS = 382;

//! constructor
CGUIFont::CGUIFont(ox::video::IVideoDriver* driver)
    : Driver(driver), Texture(0), WrongCharacter(0)
{
    Positions.reserve(RESERVED_CHARACTERS);

    if (Driver)
        Driver->grab();
}

//! destructor
CGUIFont::~CGUIFont()
{
    if (Driver)
        Driver->drop();

    if (Texture)
        Texture->drop();
}

//! loads a font file
bool CGUIFont::load(ox::io::IReadFile* file)
{
    if (!Driver)
        return false;

    return loadTexture(Driver->getTexture(file));
}

//! loads a font file
bool CGUIFont::load(const char* filename)
{
    if (!Driver)
        return false;

    return loadTexture(Driver->getTexture(filename));
}

//! load & prepare font from ITexture
bool CGUIFont::loadTexture(ox::video::ITexture* texture)
{
    if (!texture)
        return false;

    Texture = texture;
    Texture->grab();

    int lowerRightPositions = 0;

    switch (texture->getColorFormat())
    {
    case ox::video::ECF_A1R5G5B5:
        readPositions16bit(texture, lowerRightPositions);
        break;
    case ox::video::ECF_A8R8G8B8:
        readPositions32bit(texture, lowerRightPositions);
        break;
    default:
        os::Printer::log("Unsupported font texture color format.", ox::event::ELL_ERROR);
        return false;
    }

    if (Positions.size() > 127)
        WrongCharacter = 127;

    return (!Positions.empty() && lowerRightPositions);
}

void CGUIFont::readPositions32bit(ox::video::ITexture* texture, int& lowerRightPositions)
{
    int pitch = texture->getPitch();
    ox::core::CDimension2d<int> size = texture->getOriginalSize();

    int* p = (int*)texture->lock();
    if (!p)
    {
        os::Printer::log("Could not lock texture while preparing texture for a font.", ox::event::ELL_ERROR);
        return;
    }

    int colorTopLeft = *p;
    int colorLowerRight = *(p + 1);
    int colorBackGround = *(p + 2);
    int colorBackGroundWithAlphaFalse = (0x00 << 24) | (~(0xFF << 24) & colorBackGround);

    *(p + 1) = colorBackGround;
    *(p + 2) = colorBackGround;

    // start parsing

    ox::core::CPosition2d<int> pos(0, 0);

    char* row = (char*)((void*)p);

    for (pos.Y = 0; pos.Y < size.Height; ++pos.Y)
    {
        p = (int*)((void*)row);

        for (pos.X = 0; pos.X < size.Width; ++pos.X)
        {
            if (*p == colorTopLeft)
            {
                *p = colorBackGroundWithAlphaFalse;
                Positions.push_back(ox::core::CRect<int>(pos, pos));
            }
            else if (*p == colorLowerRight)
            {
                if (Positions.size() <= (unsigned int)lowerRightPositions)
                {
                    texture->unlock();
                    lowerRightPositions = 0;
                    return;
                }

                *p = colorBackGroundWithAlphaFalse;
                Positions[lowerRightPositions].LowerRightCorner = pos;
                ++lowerRightPositions;
            }
            else if (*p == colorBackGround)
                *p = colorBackGroundWithAlphaFalse;

            ++p;
        }

        row += pitch;
    }

    // Positions parsed.

    texture->unlock();

    // output warnings
    if (!lowerRightPositions || !Positions.size())
        os::Printer::log("The amount of upper corner pixels or lower corner pixels is == 0, font file may be corrupted.",
            ox::event::ELL_ERROR);
    else if (lowerRightPositions != (int)Positions.size())
        os::Printer::log("The amount of upper corner pixels and the lower corner pixels is not equal, font file may be corrupted.",
            ox::event::ELL_ERROR);
}

void CGUIFont::readPositions16bit(ox::video::ITexture* texture, int& lowerRightPositions)
{
    int pitch = texture->getPitch();
    ox::core::CDimension2d<int> size = texture->getOriginalSize();

    short* p = (short*)texture->lock();
    if (!p)
    {
        os::Printer::log("Could not lock texture while preparing texture for a font.", ox::event::ELL_ERROR);
        return;
    }

    short colorTopLeft = *p;
    short colorLowerRight = *(p + 1);
    short colorBackGround = *(p + 2);
    short colorBackGroundWithAlphaFalse = (0x0 << 15) | (~(0x1 << 15) & colorBackGround);

    *(p + 1) = colorBackGround;
    *(p + 2) = colorBackGround;

    // start parsing

    ox::core::CPosition2d<int> pos(0, 0);

    char* row = (char*)((void*)p);

    for (pos.Y = 0; pos.Y < size.Height; ++pos.Y)
    {
        p = (short*)((void*)row);

        for (pos.X = 0; pos.X < size.Width; ++pos.X)
        {
            if (*p == colorTopLeft)
            {
                *p = colorBackGroundWithAlphaFalse;
                Positions.push_back(ox::core::CRect<int>(pos, pos));
            }
            else if (*p == colorLowerRight)
            {
                if (Positions.size() <= (unsigned int)lowerRightPositions)
                {
                    texture->unlock();
                    lowerRightPositions = 0;
                    return;
                }

                *p = colorBackGroundWithAlphaFalse;
                Positions[lowerRightPositions].LowerRightCorner = pos;
                ++lowerRightPositions;
            }
            else if (*p == colorBackGround)
                *p = colorBackGroundWithAlphaFalse;

            ++p;
        }

        row += pitch;
    }

    // Positions parsed.

    texture->unlock();

    // output warnings
    if (!lowerRightPositions || !Positions.size())
        os::Printer::log("The amount of upper corner pixels or lower corner pixels is == 0, font file may be corrupted.",
            ox::event::ELL_ERROR);
    else if (lowerRightPositions != (int)Positions.size())
        os::Printer::log("The amount of upper corner pixels and the lower corner pixels is not equal, font file may be corrupted.",
            ox::event::ELL_ERROR);
}

//! returns the dimension of a text
ox::core::CDimension2d<int> CGUIFont::getDimension(const wchar_t* text)
{
    ox::core::CDimension2d<int> dim(0, Positions[0].getHeight());

    unsigned int n;

    for (const wchar_t* p = text; *p; ++p)
    {
        n = (*p) - 32;
        if (n > Positions.size())
            n = WrongCharacter;

        dim.Width += Positions[n].getWidth();
    }

    return dim;
}

inline int CGUIFont::getWidthFromCharacter(wchar_t c)
{
    unsigned int n = c - 32;
    if (n > Positions.size())
        n = WrongCharacter;

    return Positions[n].getWidth();
}

//! draws a text and clips it to the specified rectangle if wanted
void CGUIFont::draw(const wchar_t* text, const ox::core::CRect<int>& position, ox::video::SColor color,
    ox::gui::EFontHorizontalAlign horizontal, ox::gui::EFontVerticalAlign vertical, const ox::core::CRect<int>* clip)
{
    if (!Driver)
        return;

    ox::core::CDimension2d<int> textDimension;
    ox::core::CPosition2d<int> offset = position.UpperLeftCorner;

    if (horizontal != ox::gui::EFHA_LEFT || vertical != ox::gui::EFVA_TOP)
    {
        textDimension = getDimension(text);

        if (horizontal == ox::gui::EFHA_CENTER)
            offset.X += (position.getWidth() - textDimension.Width) >> 1;
        else if (horizontal == ox::gui::EFHA_RIGHT)
            offset.X += position.getWidth() - textDimension.Width;

        if (vertical == ox::gui::EFVA_CENTER)
            offset.Y += (position.getHeight() - textDimension.Height) >> 1;
        else if (vertical == ox::gui::EFVA_BOTTOM)
            offset.Y += position.getHeight() - textDimension.Height;
    }

    unsigned int n;

    while (*text)
    {
        n = (*text) - 32;
        if (n > Positions.size())
            n = WrongCharacter;

        Driver->draw2DImage(Texture, offset, Positions[n], clip, color, true);

        offset.X += Positions[n].getWidth();

        ++text;
    }
}

//! Calculates the index of the character in the text which is on a specific position.
int CGUIFont::getCharacterFromPos(const wchar_t* text, int x)
{
    int width = 0;
    int idx = 0;

    while (text[idx])
    {
        width += getWidthFromCharacter(text[idx]);

        if (width >= x)
            return idx;

        ++idx;
    }

    return -1;
}

} // end namespace gui
} // end namespace daisy
