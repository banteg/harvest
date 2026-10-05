// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CUnicodeFont.h"
#include <stdlib.h>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! The number of character hash buckets.
static const unsigned int CHARACTER_BUCKETS = 1999;

CUnicodeFont::CUnicodeFont(ox::video::IVideoDriver* driver)
{
    Package = 0;
    Driver = driver;
    Characters = 0;
}

CUnicodeFont::~CUnicodeFont()
{
    if (Characters)
    {
        for (unsigned int i = 0; i < CHARACTER_BUCKETS; ++i)
            for (unsigned int j = 0; j < Characters[i].size(); ++j)
                if (Characters[i][j].Animation)
                    Characters[i][j].Animation->remove();

        delete [] Characters;
        Characters = 0;
    }
}

bool CUnicodeFont::load(const char* filename)
{
    if (!Driver)
        return false;

    Package = Driver->getSpritePackage(filename, true);
    if (!Package)
        return false;

    Characters = new std::vector<SUnicodeChar>[CHARACTER_BUCKETS];
    Height = 0;

    const ox::TArray<ox::core::CString<char> >& names = Package->getAnimationList();
    for (unsigned int i = 0; i < names.size(); ++i)
    {
        SUnicodeChar character;
        character.Character = strtol(names[i].c_str(), 0, 10);
        character.Size.Width = 0;
        character.Animation = 0;

        // The first character sets the height of the font.
        if (Height == 0)
        {
            character.Animation = Package->addNewAnimationState(names[i]);
            character.Size = character.Animation->getFrameOriginalSize(0);
            Height = character.Size.Height;
        }

        Characters[(unsigned int)character.Character % CHARACTER_BUCKETS].push_back(character);
    }

    MissingCharacterWidth = Height / 2;
    return true;
}

inline SUnicodeChar* CUnicodeFont::findCharacter(wchar_t character)
{
    std::vector<SUnicodeChar>& bucket = Characters[(unsigned int)character % CHARACTER_BUCKETS];
    for (unsigned int i = 0; i < bucket.size(); ++i)
        if (bucket[i].Character == character)
            return &bucket[i];

    return 0;
}

inline void CUnicodeFont::loadCharacter(SUnicodeChar* character)
{
    character->Animation = Package->addNewAnimationState(character->Character);
    if (character->Animation)
        character->Size = character->Animation->getFrameSize(0);
}

void CUnicodeFont::draw(const wchar_t* text, const ox::core::CRect<int>& position, ox::video::SColor color,
    ox::gui::EFontHorizontalAlign horizontal, ox::gui::EFontVerticalAlign vertical, const ox::core::CRect<int>* clip)
{
    ox::core::CPosition2d<int> offset = position.UpperLeftCorner;

    if (horizontal != ox::gui::EFHA_LEFT || vertical != ox::gui::EFVA_TOP)
    {
        ox::core::CDimension2d<int> textDimension = getDimension(text);

        if (horizontal == ox::gui::EFHA_CENTER)
            offset.X += (position.getWidth() - textDimension.Width) >> 1;
        else if (horizontal == ox::gui::EFHA_RIGHT)
            offset.X += position.getWidth() - textDimension.Width;

        if (vertical == ox::gui::EFVA_CENTER)
            offset.Y += (position.getHeight() - textDimension.Height) >> 1;
        else if (vertical == ox::gui::EFVA_BOTTOM)
            offset.Y += position.getHeight() - textDimension.Height;
    }

    ox::video::SColor currentColor = color;

    while (*text)
    {
        if (*text == L'#')
        {
            switch (text[1])
            {
            case L'0': currentColor = ox::video::SColor(color.getAlpha(), 0x00, 0x00, 0x00); text += 2; continue;
            case L'1': currentColor = ox::video::SColor(color.getAlpha(), 0xd5, 0x88, 0x7a); text += 2; continue;
            case L'2': currentColor = ox::video::SColor(color.getAlpha(), 0xbd, 0xcd, 0x6f); text += 2; continue;
            case L'3': currentColor = ox::video::SColor(color.getAlpha(), 0xda, 0xd5, 0x78); text += 2; continue;
            case L'4': currentColor = ox::video::SColor(color.getAlpha(), 0xd1, 0xa7, 0x44); text += 2; continue;
            case L'5': currentColor = ox::video::SColor(color.getAlpha(), 0xa0, 0x80, 0x9a); text += 2; continue;
            case L'6': currentColor = ox::video::SColor(color.getAlpha(), 0xb5, 0xc7, 0xee); text += 2; continue;
            case L'7': currentColor = ox::video::SColor(color.getAlpha(), 0xff, 0xff, 0xff); text += 2; continue;
            case L'8': currentColor = ox::video::SColor(color.getAlpha(), 0x9f, 0x6a, 0x5c); text += 2; continue;
            case L'9': currentColor = ox::video::SColor(color.getAlpha(), 0xf0, 0xe7, 0xb7); text += 2; continue;
            case L'o': currentColor = OriginalColor; text += 2; continue;
            }
        }

        SUnicodeChar* character = findCharacter(*text);
        if (character)
        {
            if (!character->Animation)
                loadCharacter(character);

            character->Animation->draw(offset, clip, currentColor);
            offset.X += character->Size.Width;
        }
        else
            offset.X += MissingCharacterWidth;

        ++text;
    }

    RecentColor = currentColor;
}

ox::core::CDimension2d<int> CUnicodeFont::getDimension(const wchar_t* text)
{
    int width = 0;

    while (*text)
    {
        if (*text == L'#')
        {
            switch (text[1])
            {
            case L'0':
            case L'1':
            case L'2':
            case L'3':
            case L'4':
            case L'5':
            case L'6':
            case L'7':
            case L'8':
            case L'9':
            case L'o':
                text += 2;
                continue;
            }
        }

        SUnicodeChar* character = findCharacter(*text);
        if (character)
        {
            if (character->Size.Width == 0)
                loadCharacter(character);
            width += character->Size.Width;
        }
        else
            width += MissingCharacterWidth;

        ++text;
    }

    return ox::core::CDimension2d<int>(width, Height);
}

int CUnicodeFont::getCharacterFromPos(const wchar_t* text, int x)
{
    int index = 0;

    while (*text)
    {
        if (*text == L'#')
        {
            switch (text[1])
            {
            case L'0':
            case L'1':
            case L'2':
            case L'3':
            case L'4':
            case L'5':
            case L'6':
            case L'7':
            case L'8':
            case L'9':
            case L'o':
                text += 2;
                continue;
            }
        }

        SUnicodeChar* character = findCharacter(*text);
        if (character)
        {
            if (character->Size.Width == 0)
                loadCharacter(character);
            x -= character->Size.Width;
        }
        else
            x -= MissingCharacterWidth;

        if (x < 0)
            return index;

        ++text;
        ++index;
    }

    return -1;
}

void CUnicodeFont::setOriginalColor(const ox::video::SColor& color)
{
    OriginalColor = color;
}

ox::video::SColor CUnicodeFont::getRecentColor()
{
    return RecentColor;
}

} // end namespace gui
} // end namespace daisy
