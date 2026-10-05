// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIStaticText.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUISTATICTEXT_H
#define DAISY_GUI_CGUISTATICTEXT_H

#include "ox/gui/IGUIStaticText.h"
#include "ox/TArray.h"

namespace ox {
namespace video { class ISpriteAnimationState; }
}

namespace daisy {
namespace gui {

//! Multi-line text with an optional paragraph icon, revealed letter by letter on request.
class CGUIStaticText : public ox::gui::IGUIStaticText
{
public:
    //! constructor; lines broken by word wrap start with linePrefix
    CGUIStaticText(const wchar_t* text, bool border, ox::gui::IGUIEnvironment* environment,
        ox::gui::IGUIElement* parent, int id, const ox::core::CRect<int>& rectangle, const wchar_t* linePrefix);

    //! destructor
    ~CGUIStaticText();

    //! draws the element and its children
    virtual void draw();

    //! Sets another skin independent font.
    virtual void setOverrideFont(ox::gui::IGUIFont* font = 0);

    //! Sets another color for the text.
    virtual void setOverrideColor(ox::video::SColor color);

    virtual ox::video::SColor getOverrideColor();

    //! Sets if the static text should use the overide color or the
    //! color in the gui skin.
    virtual void enableOverrideColor(bool enable);

    //! Enables or disables word wrap for using the static text as
    //! multiline text control.
    virtual void setWordWrap(bool enable);

    virtual void setTextAlignment(ox::gui::EFontHorizontalAlign horizontal, ox::gui::EFontVerticalAlign vertical);

    //! Sets the new caption of this element.
    virtual void setText(const wchar_t* text);

    //! Returns the height of the text in pixels when it is drawn.
    virtual int getTextHeight();

    virtual ox::core::CDimension2d<int> getPreferredSize();

    //! Shows the named animation of the package beside the first lines. A name ending in "|right"
    //! puts the icon at the right edge; resize grows the element to fit the text and the icon.
    virtual void setParagraphIcon(const char* name, ox::video::ISpritePackage* package, bool resize);

    //! Starts revealing the text after the given number of milliseconds.
    virtual void activateProgressiveReveal(unsigned int time);

    //! The milliseconds the reveal of the whole text takes.
    virtual unsigned int getTotalProgressiveTime();

    //! Scrolls centered text that does not fit back and forth.
    virtual void activateOffsetScrollingToEnsureVisibleText();

    //! Shrinks the element to the size of its text.
    virtual void packSize();

private:
    //! Breaks the text into lines, the first ones beside the paragraph icon.
    void breakText();

    bool Border;
    bool OverrideColorEnabled;
    bool WordWrap;
    ox::gui::EFontHorizontalAlign HAlign;
    ox::gui::EFontVerticalAlign VAlign;
    ox::core::CString<wchar_t> LinePrefix;
    ox::video::SColor OverrideColor;
    ox::gui::IGUIFont* OverrideFont;
    ox::gui::IGUIFont* LastBreakFont; // stored because: if skin changes, line break must be recalculated.

    ox::TArray<ox::core::CString<wchar_t> > BrokenText;

    ox::video::ISpriteAnimationState* ParagraphIcon;
    //! The number of lines beside the paragraph icon.
    int IconLines;
    bool IconRight;
    bool ProgressiveReveal;
    //! The Timer::getTime at which the reveal starts.
    unsigned int RevealStart;
    bool OffsetScrolling;
};

} // end namespace gui
} // end namespace daisy

#endif
