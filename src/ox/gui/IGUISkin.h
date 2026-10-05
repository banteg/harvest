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

enum EGUI_DEFAULT_COLOR {};
enum EGUI_DEFAULT_SIZE {};
enum EGUI_DEFAULT_TEXT {};

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
