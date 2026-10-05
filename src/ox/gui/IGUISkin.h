// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUISkin.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUISkin; the color and size enumerators are not recovered yet.

#ifndef OX_GUI_IGUISKIN_H
#define OX_GUI_IGUISKIN_H

#include "../IUnknown.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIFont;

//! Default colors, numbered as in Irrlicht 0.7; Oxeye's additions after EGDC_WINDOW are not named yet.
enum EGUI_DEFAULT_COLOR
{
    EGDC_3D_DARK_SHADOW = 0,
    EGDC_3D_SHADOW,
    EGDC_3D_FACE,
    EGDC_3D_HIGH_LIGHT,
    EGDC_3D_LIGHT,
    EGDC_ACTIVE_BORDER,
    EGDC_ACTIVE_CAPTION,
    EGDC_APP_WORKSPACE,
    EGDC_BUTTON_TEXT,
    EGDC_GRAY_TEXT,
    EGDC_HIGH_LIGHT,
    EGDC_HIGH_LIGHT_TEXT,
    EGDC_INACTIVE_BORDER,
    EGDC_INACTIVE_CAPTION,
    EGDC_TOOLTIP,
    EGDC_SCROLLBAR,
    EGDC_WINDOW
};
//! Default sizes. Partial: only the values recovered code uses, numbered as in Irrlicht 0.7.
enum EGUI_DEFAULT_SIZE
{
    EGDS_SCROLLBAR_SIZE = 0,
    EGDS_MENU_HEIGHT,
    EGDS_WINDOW_BUTTON_WIDTH
};
//! The default texts of message box buttons, as in Irrlicht 0.7.
enum EGUI_DEFAULT_TEXT
{
    EGDT_MSG_BOX_OK = 0,
    EGDT_MSG_BOX_CANCEL,
    EGDT_MSG_BOX_YES,
    EGDT_MSG_BOX_NO
};

//! A skin modifies the look of the GUI elements.
class IGUISkin : public IUnknown
{
public:
    virtual video::SColor getColor(EGUI_DEFAULT_COLOR color) = 0;
    virtual void setColor(EGUI_DEFAULT_COLOR which, video::SColor newColor) = 0;
    virtual int getSize(EGUI_DEFAULT_SIZE size) = 0;
    virtual const wchar_t* getDefaultText(EGUI_DEFAULT_TEXT text) = 0;
    virtual void setDefaultText(EGUI_DEFAULT_TEXT which, const wchar_t* newText) = 0;
    virtual void setSize(EGUI_DEFAULT_SIZE which, int size) = 0;
    virtual IGUIFont* getFont() = 0;
    virtual void setFont(IGUIFont* font) = 0;
    virtual video::ISpritePackage* getSpritePackage() = 0;
    virtual void setSpritePackage(video::ISpritePackage* package) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
