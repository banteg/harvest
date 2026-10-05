// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The inline IGUIStaticText text helpers. The Linux build emits breakText's copy in
// daisy/gui/CGUIEnvironment.cpp. They are kept out of IGUIStaticText.h because instantiating their
// templates there changes the register choices of the game units that include it.

#ifndef OX_GUI_IGUISTATICTEXTINLINE_H
#define OX_GUI_IGUISTATICTEXTINLINE_H

#include "IGUIStaticText.h"
#include "../TArray.h"

namespace ox {
namespace gui {

inline void IGUIStaticText::breakText(const core::CString<wchar_t>& text, IGUIFont* font, TArray<core::CString<wchar_t> >& lines,
    int width, const wchar_t* style)
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

        // the terminating zero flushes the last word
        for (int i = 0; i <= text.size(); ++i)
        {
            wchar_t c = text[i];
            bool lineBreak = c == L'\n';
            if (lineBreak)
                c = L' ';

            if (c == 0 || c == L' ')
            {
                if (word.size() != 0)
                {
                    int whitespaceLength = font->getDimension(whitespace.c_str()).Width;
                    int wordLength = font->getDimension(word.c_str()).Width;
                    length += whitespaceLength + wordLength;

                    if (length <= width || wordLength >= width)
                    {
                        line.append(whitespace);
                        line.append(word);
                    }
                    else
                    {
                        lines.push_back(line);
                        length = font->getDimension(style).Width + wordLength;
                        line = style;
                        line.append(word);
                    }

                    word = L"";
                    whitespace = L"";
                }

                if (c != 0)
                    whitespace.append(c);

                if (lineBreak)
                {
                    line.append(whitespace);
                    line.append(word);
                    lines.push_back(line);
                    line = L"";
                    word = L"";
                    length = 0;
                    whitespace = L"";
                }
            }
            else
                word.append(c);
        }

        line.append(whitespace);
        line.append(word);
        lines.push_back(line);
    }
    else
        lines.push_back(text);
}

inline int IGUIStaticText::getMultilineHeight(const core::CString<wchar_t>& text, IGUIFont* font, int width,
    const wchar_t* style)
{
    TArray<core::CString<wchar_t> > lines;
    breakText(text, font, lines, width, style);
    return lines.size() * font->getDimension(L"A").Height;
}

} // end namespace gui
} // end namespace ox

#endif
