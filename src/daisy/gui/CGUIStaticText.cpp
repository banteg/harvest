// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIStaticText.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye adds text alignment, a paragraph icon beside the first lines, a progressive reveal and
// back-and-forth scrolling of centered text that does not fit.

#include "CGUIStaticText.h"
#include "ox/gui/IGUIElementInline.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/ISpriteAnimationState.h"
#include "daisy/os.h"
#include <math.h>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! Pixels of text revealed per second.
static const float REVEAL_SPEED = 300.0f;
static const float PI = 3.14159274f;
//! Milliseconds of each phase of the offset scrolling.
static const unsigned int SCROLL_PHASE_TIME = 2000;

//! Clips rect against other, as Irrlicht's rect::clipAgainst.
static inline void clipAgainst(ox::core::CRect<int>& rect, const ox::core::CRect<int>& other)
{
    if (other.LowerRightCorner.X < rect.LowerRightCorner.X)
        rect.LowerRightCorner.X = other.LowerRightCorner.X;
    if (other.LowerRightCorner.Y < rect.LowerRightCorner.Y)
        rect.LowerRightCorner.Y = other.LowerRightCorner.Y;

    if (other.UpperLeftCorner.X > rect.UpperLeftCorner.X)
        rect.UpperLeftCorner.X = other.UpperLeftCorner.X;
    if (other.UpperLeftCorner.Y > rect.UpperLeftCorner.Y)
        rect.UpperLeftCorner.Y = other.UpperLeftCorner.Y;
}

//! constructor
CGUIStaticText::CGUIStaticText(const wchar_t* text, bool border, ox::gui::IGUIEnvironment* environment,
    ox::gui::IGUIElement* parent, int id, const ox::core::CRect<int>& rectangle, const wchar_t* linePrefix)
    : IGUIStaticText(environment, parent, id, rectangle), Border(border), OverrideColorEnabled(false),
      WordWrap(false), HAlign(ox::gui::EFHA_LEFT), VAlign(ox::gui::EFVA_TOP), LinePrefix(linePrefix),
      OverrideFont(0), LastBreakFont(0), ParagraphIcon(0), IconLines(0), ProgressiveReveal(false),
      OffsetScrolling(false)
{
    OverrideColor = ox::video::SColor(101, 255, 255, 255);
    Text = text;
}

//! destructor
CGUIStaticText::~CGUIStaticText()
{
    if (OverrideFont)
        OverrideFont->drop();

    if (ParagraphIcon)
        ParagraphIcon->remove();
}

//! draws the element and its children
void CGUIStaticText::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    ox::core::CRect<int> frameRect(AbsoluteRect);

    // draw the border

    if (Border)
    {
        frameRect.LowerRightCorner.Y = frameRect.UpperLeftCorner.Y + 1;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), frameRect, &AbsoluteClippingRect);

        frameRect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
        frameRect.LowerRightCorner.X = frameRect.UpperLeftCorner.X + 1;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), frameRect, &AbsoluteClippingRect);

        frameRect = AbsoluteRect;
        frameRect.UpperLeftCorner.X = frameRect.LowerRightCorner.X - 1;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

        frameRect = AbsoluteRect;
        frameRect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - 1;
        frameRect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

        frameRect = AbsoluteRect;
        frameRect.UpperLeftCorner.X += 3;
    }

    // draw the text
    if (Text.size())
    {
        ox::gui::IGUIFont* font = OverrideFont;
        if (!OverrideFont)
            font = skin->getFont();

        ox::core::CRect<int> revealClip = AbsoluteClippingRect;
        int iconWidth = 0;
        if (ParagraphIcon)
            iconWidth = ParagraphIcon->getFrameSize(0).Width;

        // the line being revealed and the clip rectangle that reveals it
        int revealLine = 0;
        if (ProgressiveReveal)
        {
            if (daisy::os::Timer::getTime() <= RevealStart)
            {
                IGUIElement::draw();
                return;
            }

            int revealed = (int)((float)(daisy::os::Timer::getTime() - RevealStart) * REVEAL_SPEED * 0.001f);
            int lineWidth = RelativeRect.getWidth();
            while (revealed > lineWidth && revealLine < (int)BrokenText.size())
            {
                ++revealLine;
                revealed -= lineWidth;
                lineWidth = RelativeRect.getWidth();
                if (revealLine < IconLines)
                    lineWidth = RelativeRect.getWidth() - 6 - iconWidth;
            }

            revealClip.LowerRightCorner.X = revealed + revealClip.UpperLeftCorner.X;
            if (!IconRight && revealLine != 0 && revealLine < IconLines)
                revealClip.LowerRightCorner.X += iconWidth + 6;

            clipAgainst(revealClip, AbsoluteClippingRect);
        }

        if (font)
        {
            if (!WordWrap)
            {
                if (ProgressiveReveal && revealLine == 0)
                {
                    font->draw(Text.c_str(), frameRect,
                        OverrideColorEnabled ? OverrideColor : skin->getColor(ox::gui::EGDC_BUTTON_TEXT),
                        ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, &revealClip);
                }
                else if (!OffsetScrolling || HAlign != ox::gui::EFHA_CENTER)
                {
                    font->draw(Text.c_str(), frameRect,
                        OverrideColorEnabled ? OverrideColor : skin->getColor(ox::gui::EGDC_BUTTON_TEXT),
                        HAlign, VAlign, &AbsoluteClippingRect);
                }
                else
                {
                    // scroll text that does not fit to each side and back
                    int offset = 0;
                    int textWidth = font->getDimension(Text.c_str()).Width;
                    if (textWidth > frameRect.getWidth())
                    {
                        unsigned int time = daisy::os::Timer::getTime();
                        offset = frameRect.getWidth() - textWidth;
                        unsigned int phase = (time / SCROLL_PHASE_TIME) & 3;
                        if (phase != 0)
                        {
                            if (phase == 1)
                                offset = (int)((float)offset * cos((time % SCROLL_PHASE_TIME) * 0.0005f * PI));
                            else if (phase == 2)
                                offset = -offset;
                            else
                                offset = -(int)((float)offset * cos((time % SCROLL_PHASE_TIME) * 0.0005f * PI));
                        }
                    }

                    font->draw(Text.c_str(),
                        ox::core::CRect<int>(frameRect.UpperLeftCorner.X + offset, frameRect.UpperLeftCorner.Y,
                            frameRect.LowerRightCorner.X + offset, frameRect.LowerRightCorner.Y),
                        OverrideColorEnabled ? OverrideColor : skin->getColor(ox::gui::EGDC_BUTTON_TEXT),
                        HAlign, VAlign, &AbsoluteClippingRect);
                }
            }
            else
            {
                if (font != LastBreakFont)
                    breakText();

                ox::core::CRect<int> r = frameRect;
                int height = font->getDimension(L"A").Height;
                ox::video::SColor color =
                    OverrideColorEnabled ? OverrideColor : skin->getColor(ox::gui::EGDC_BUTTON_TEXT);
                font->setOriginalColor(color);

                if (ParagraphIcon)
                {
                    int iconX = 0;
                    if (IconRight)
                        iconX = frameRect.getWidth() - iconWidth;

                    if (ProgressiveReveal && revealLine == 0)
                        ParagraphIcon->draw(ox::core::CPosition2d<int>(r.UpperLeftCorner.X + iconX, r.UpperLeftCorner.Y) -
                            ParagraphIcon->getFrameOffset(0), &revealClip, ox::video::SColor(0xffffffff));
                    else
                        ParagraphIcon->draw(ox::core::CPosition2d<int>(r.UpperLeftCorner.X + iconX, r.UpperLeftCorner.Y) -
                            ParagraphIcon->getFrameOffset(0), &AbsoluteClippingRect, ox::video::SColor(0xffffffff));
                }

                if (VAlign == ox::gui::EFVA_CENTER)
                {
                    int offset = (r.getHeight() - BrokenText.size() * height) / 2;
                    r.LowerRightCorner.Y += offset;
                    r.UpperLeftCorner.Y += offset;
                }

                for (unsigned int i = 0; i < BrokenText.size(); ++i)
                {
                    int indent = 0;
                    if (IconLines > i && !IconRight)
                        indent = iconWidth + 6;

                    if (ProgressiveReveal && revealLine == (int)i)
                        font->draw(BrokenText[i].c_str(), ox::core::CRect<int>(r.UpperLeftCorner.X + indent,
                            r.UpperLeftCorner.Y, r.LowerRightCorner.X, r.LowerRightCorner.Y), color, HAlign,
                            ox::gui::EFVA_TOP, &revealClip);
                    else
                        font->draw(BrokenText[i].c_str(), ox::core::CRect<int>(r.UpperLeftCorner.X + indent,
                            r.UpperLeftCorner.Y, r.LowerRightCorner.X, r.LowerRightCorner.Y), color, HAlign,
                            ox::gui::EFVA_TOP, &AbsoluteClippingRect);

                    color = font->getRecentColor();

                    if (ProgressiveReveal && revealLine <= (int)i)
                        break;

                    r.UpperLeftCorner.Y += height;
                    r.LowerRightCorner.Y += height;
                }
            }
        }
    }

    IGUIElement::draw();
}

//! Breaks the text into lines, the first ones beside the paragraph icon.
void CGUIStaticText::breakText()
{
    ox::gui::IGUISkin* skin = Environment->getSkin();

    if (!WordWrap || !skin)
        return;

    BrokenText.clear();

    ox::gui::IGUIFont* font = OverrideFont;
    if (!OverrideFont)
        font = skin->getFont();

    if (!font)
        return;

    LastBreakFont = font;

    if (!ParagraphIcon)
    {
        IGUIStaticText::breakText(Text, font, BrokenText, RelativeRect.getWidth(), LinePrefix.c_str());
        return;
    }

    // the lines beside the icon are narrower
    ox::core::CDimension2d<int> iconSize = ParagraphIcon->getFrameSize(0);
    ox::core::CDimension2d<int> charSize = font->getDimension(L"A");
    int iconLines = (iconSize.Height + charSize.Height / 2 + 6) / charSize.Height;
    if (iconLines <= 0)
        iconLines = 1;

    ox::TArray<ox::core::CString<wchar_t> > iconText;
    ox::TArray<ox::core::CString<wchar_t> > restText;
    IGUIStaticText::breakText(Text, font, iconText, RelativeRect.getWidth() - iconSize.Width - 6, L"");

    if (iconLines < (int)iconText.size())
    {
        // break the rest again at the full width
        ox::core::CString<wchar_t> rest;
        for (unsigned int i = iconLines; i < iconText.size(); ++i)
        {
            rest.append(iconText[i]);
            rest.append(ox::core::CString<wchar_t>(L" "));
        }

        IGUIStaticText::breakText(rest, font, restText, RelativeRect.getWidth(), L"");
    }

    for (unsigned int i = 0; i < (unsigned int)iconLines && i < iconText.size(); ++i)
        BrokenText.push_back(iconText[i]);

    for (unsigned int i = 0; i < restText.size(); ++i)
        BrokenText.push_back(restText[i]);

    IconLines = BrokenText.size();
    if (IconLines > iconLines)
        IconLines = iconLines;
}

void CGUIStaticText::setTextAlignment(ox::gui::EFontHorizontalAlign horizontal, ox::gui::EFontVerticalAlign vertical)
{
    HAlign = horizontal;
    VAlign = vertical;
}

//! Sets another skin independent font.
void CGUIStaticText::setOverrideFont(ox::gui::IGUIFont* font)
{
    if (OverrideFont)
        OverrideFont->drop();

    OverrideFont = font;

    if (OverrideFont)
        OverrideFont->grab();

    breakText();
}

ox::video::SColor CGUIStaticText::getOverrideColor()
{
    return OverrideColor;
}

//! Sets another color for the text.
void CGUIStaticText::setOverrideColor(ox::video::SColor color)
{
    OverrideColor = color;
    OverrideColorEnabled = true;
}

//! Sets if the static text should use the overide color or the
//! color in the gui skin.
void CGUIStaticText::enableOverrideColor(bool enable)
{
    OverrideColorEnabled = enable;
}

//! Enables or disables word wrap for using the static text as
//! multiline text control.
void CGUIStaticText::setWordWrap(bool enable)
{
    WordWrap = enable;
    breakText();
}

ox::core::CDimension2d<int> CGUIStaticText::getPreferredSize()
{
    ox::gui::IGUIFont* font = OverrideFont;
    if (!font)
        font = Environment->getSkin()->getFont();

    if (font && Text.size() > 0 && BrokenText.empty())
        return font->getDimension(Text.c_str());

    return ox::core::CDimension2d<int>(RelativeRect.getWidth(), RelativeRect.getHeight());
}

//! Sets the new caption of this element.
void CGUIStaticText::setText(const wchar_t* text)
{
    IGUIElement::setText(text);
    breakText();
}

//! Returns the height of the text in pixels when it is drawn.
int CGUIStaticText::getTextHeight()
{
    ox::gui::IGUISkin* skin = Environment->getSkin();

    if (!skin)
        return 0;

    ox::gui::IGUIFont* font = OverrideFont;
    if (!OverrideFont)
        font = skin->getFont();

    if (!font)
        return 0;

    int height = font->getDimension(L"A").Height;

    if (WordWrap)
        height *= BrokenText.size();

    return height;
}

void CGUIStaticText::setParagraphIcon(const char* name, ox::video::ISpritePackage* package, bool resize)
{
    if (ParagraphIcon)
    {
        ParagraphIcon->remove();
        ParagraphIcon = 0;
        IconLines = 0;
    }

    ox::core::CString<char> animation = name;
    IconRight = false;
    int pos = animation.findNext("|right", 0);
    if (pos > 0)
    {
        IconRight = true;
        animation = animation.subString(0, pos);
    }

    if (package)
        ParagraphIcon = package->addNewAnimationState(animation.c_str());

    breakText();

    if (resize)
    {
        int height = 0;
        ox::gui::IGUIFont* font = OverrideFont;
        if (!font)
            font = Environment->getSkin()->getFont();
        if (font)
            height = font->getDimension(L"A").Height * BrokenText.size();

        if (ParagraphIcon)
        {
            int iconHeight = ParagraphIcon->getFrameSize(0).Height;
            if (iconHeight >= height)
                height = iconHeight;
        }

        if (height > 0)
            setRelativePosition(ox::core::CRect<int>(RelativeRect.UpperLeftCorner.X, RelativeRect.UpperLeftCorner.Y,
                RelativeRect.LowerRightCorner.X, RelativeRect.UpperLeftCorner.Y + height));
    }
}

//! Shrinks the element to the size of its text.
void CGUIStaticText::packSize()
{
    ox::gui::IGUIFont* font = OverrideFont;
    if (!font)
        font = Environment->getSkin()->getFont();

    if (!WordWrap)
    {
        ox::core::CDimension2d<int> dim = font->getDimension(Text.c_str());
        setRelativePosition(ox::core::CRect<int>(RelativeRect.UpperLeftCorner, dim));
    }
    else
    {
        int height = font->getDimension(L"A").Height * BrokenText.size();
        int width = 0;
        for (unsigned int i = 0; i < BrokenText.size(); ++i)
        {
            int lineWidth = font->getDimension(BrokenText[i].c_str()).Width;
            if (lineWidth > width)
                width = lineWidth;
        }

        setRelativePosition(ox::core::CRect<int>(RelativeRect.UpperLeftCorner.X, RelativeRect.UpperLeftCorner.Y,
            RelativeRect.UpperLeftCorner.X + width, RelativeRect.UpperLeftCorner.Y + height));
    }
}

//! Starts revealing the text after the given number of milliseconds.
void CGUIStaticText::activateProgressiveReveal(unsigned int time)
{
    ProgressiveReveal = true;
    RevealStart = daisy::os::Timer::getTime() + time;
}

void CGUIStaticText::activateOffsetScrollingToEnsureVisibleText()
{
    OffsetScrolling = true;
}

//! The milliseconds the reveal of the whole text takes.
unsigned int CGUIStaticText::getTotalProgressiveTime()
{
    if (BrokenText.empty())
        return (unsigned int)((float)RelativeRect.getWidth() / REVEAL_SPEED * 1000.0f);

    if (IconLines > 0 && ParagraphIcon)
    {
        int iconWidth = ParagraphIcon->getFrameSize(0).Width;
        return (IconLines - 1) * (unsigned int)((float)(RelativeRect.getWidth() - iconWidth - 6) / REVEAL_SPEED * 1000.0f) +
            ((int)BrokenText.size() + 1 - IconLines) * (unsigned int)((float)RelativeRect.getWidth() / REVEAL_SPEED * 1000.0f);
    }

    return BrokenText.size() * (unsigned int)((float)RelativeRect.getWidth() / REVEAL_SPEED * 1000.0f);
}

} // end namespace gui
} // end namespace daisy
