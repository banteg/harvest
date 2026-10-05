// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUISkin.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUISKIN_H
#define DAISY_GUI_CGUISKIN_H

#include "ox/gui/IGUISkin.h"
#include "ox/core/CString.h"

namespace daisy {
namespace gui {

//! The default skin. Oxeye adds a sprite package that the widgets draw with and that sets some sizes.
class CGUISkin : public ox::gui::IGUISkin
{
public:
    CGUISkin(ox::gui::EGUI_SKIN_TYPE type);
    virtual ~CGUISkin();

    //! returns default color
    virtual ox::video::SColor getColor(ox::gui::EGUI_DEFAULT_COLOR color);

    //! sets a default color
    virtual void setColor(ox::gui::EGUI_DEFAULT_COLOR which, ox::video::SColor newColor);

    //! returns default size
    virtual int getSize(ox::gui::EGUI_DEFAULT_SIZE size);

    //! Returns a default text, for example the message box button captions.
    virtual const wchar_t* getDefaultText(ox::gui::EGUI_DEFAULT_TEXT text);

    //! Sets a default text, for example the message box button captions.
    virtual void setDefaultText(ox::gui::EGUI_DEFAULT_TEXT which, const wchar_t* newText);

    //! sets a default size
    virtual void setSize(ox::gui::EGUI_DEFAULT_SIZE which, int size);

    //! returns the default font
    virtual ox::gui::IGUIFont* getFont();

    //! sets a default font
    virtual void setFont(ox::gui::IGUIFont* font);

    virtual ox::video::ISpritePackage* getSpritePackage();

    //! Sets the package the widgets draw their sprites from and takes the button, scroll bar and
    //! check box sizes from its sprites.
    virtual void setSpritePackage(ox::video::ISpritePackage* package);

private:
    ox::video::SColor Colors[ox::gui::EGDC_COUNT];
    int Sizes[ox::gui::EGDS_COUNT];
    ox::gui::IGUIFont* Font;
    ox::core::CString<wchar_t> Texts[ox::gui::EGDT_COUNT];
    ox::video::ISpritePackage* SpritePackage;
};

} // end namespace gui
} // end namespace daisy

#endif
