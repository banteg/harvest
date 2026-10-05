// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIStaticText.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIStaticText.

#ifndef OX_GUI_IGUISTATICTEXT_H
#define OX_GUI_IGUISTATICTEXT_H

#include "IGUIElement.h"
#include "IGUIFont.h"
#include "../video/SColor.h"
#include "../TArray.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

//! Multi-line text, optionally revealed letter by letter.
class IGUIStaticText : public IGUIElement
{
public:
    virtual void setOverrideFont(IGUIFont* font) = 0;
    virtual void setOverrideColor(video::SColor color) = 0;
    virtual video::SColor getOverrideColor() = 0;
    virtual void enableOverrideColor(bool enable) = 0;
    virtual void setWordWrap(bool enable) = 0;
    virtual void setTextAlignment(EFontHorizontalAlign horizontal, EFontVerticalAlign vertical) = 0;
    virtual int getTextHeight() = 0;
    //! Shows the named animation of the package beside the text.
    virtual void setParagraphIcon(const char* name, video::ISpritePackage* package, bool animated) = 0;
    //! Reveals the text over the given number of milliseconds.
    virtual void activateProgressiveReveal(unsigned int time) = 0;
    virtual unsigned int getTotalProgressiveTime() = 0;
    virtual void activateOffsetScrollingToEnsureVisibleText() = 0;
    //! Shrinks the element to the size of its text.
    virtual void packSize() = 0;

    //! Splits text at spaces and newlines into the lines that fit width when drawn with font; every
    //! wrapped line starts with style. A word wider than width gets a line of its own.
    static void breakText(const core::CString<wchar_t>& text, IGUIFont* font, TArray<core::CString<wchar_t> >& lines,
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

    //! The height of text broken into lines of width.
    static int getMultilineHeight(const core::CString<wchar_t>& text, IGUIFont* font, int width,
        const wchar_t* style)
    {
        TArray<core::CString<wchar_t> > lines;
        breakText(text, font, lines, width, style);
        return lines.size() * font->getDimension(L"A").Height;
    }
};

} // end namespace gui
} // end namespace ox

#endif
