// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The inline IGUIStaticText::breakText. The Linux build emits its copy in daisy/gui/CGUIEnvironment.cpp.
// It is kept out of IGUIStaticText.h because instantiating its templates there changes the register
// choices of the game units that include it.

#ifndef OX_GUI_IGUISTATICTEXTINLINE_H
#define OX_GUI_IGUISTATICTEXTINLINE_H

#include "IGUIStaticText.h"
#include "../TArray.h"

namespace ox {
namespace gui {

inline void IGUIStaticText::breakText(const core::CString<wchar_t>& text, IGUIFont* font,
    TArray<core::CString<wchar_t> >& lines, int width, const wchar_t* style)
{
    if (!font)
        return;

    lines.clear();

    if (width > 4)
    {
        core::CString<wchar_t> line;
        core::CString<wchar_t> word;
        core::CString<wchar_t> whitespace;
        int length = 0;

        int textSize = text.size();
        wchar_t character;

        for (int i = 0; i < textSize; ++i)
        {
            character = text[i];
            bool lineBreak = false;
            if (character == L'\n')
            {
                lineBreak = true;
                character = L' ';
            }

            if (character == 0 || character == L' ')
            {
                if (word.size())
                {
                    int whitespaceLength = font->getDimension(whitespace.c_str()).Width;
                    int wordLength = font->getDimension(word.c_str()).Width;
                    length += whitespaceLength + wordLength;

                    if (length <= width || wordLength >= width)
                    {
                        line += whitespace;
                        line += word;
                    }
                    else
                    {
                        lines.push_back(line);
                        length = font->getDimension(style).Width + wordLength;
                        line = style;
                        line += word;
                    }

                    word = L"";
                    whitespace = L"";
                }

                if (character != 0)
                    whitespace += character;

                if (lineBreak)
                {
                    line += whitespace;
                    line += word;
                    lines.push_back(line);
                    line = L"";
                    word = L"";
                    length = 0;
                    whitespace = L"";
                }
            }
            else
                word += character;
        }

        line += whitespace;
        line += word;
        lines.push_back(line);
    }
    else
        lines.push_back(text);
}

} // end namespace gui
} // end namespace ox

#endif
