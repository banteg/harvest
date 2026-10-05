// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUISkin.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye makes the colors opaque, adds three colors, two sizes and the sprite package.

#include "CGUISkin.h"
#include "ox/gui/IGUIFont.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/ISpriteAnimationState.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

using namespace ox::gui;

//! returns default color
ox::video::SColor CGUISkin::getColor(EGUI_DEFAULT_COLOR color)
{
    return Colors[color];
}

//! sets a default color
void CGUISkin::setColor(EGUI_DEFAULT_COLOR which, ox::video::SColor newColor)
{
    if (which >= 0 && which <= EGDC_COUNT)
        Colors[which] = newColor;
}

//! returns default size
int CGUISkin::getSize(EGUI_DEFAULT_SIZE size)
{
    return Sizes[size];
}

//! sets a default size
void CGUISkin::setSize(EGUI_DEFAULT_SIZE which, int size)
{
    if (which >= 0 && which <= EGDS_COUNT)
        Sizes[which] = size;
}

//! returns the default font
IGUIFont* CGUISkin::getFont()
{
    return Font;
}

//! Returns a default text. For example for Message box button captions:
//! "OK", "Cancel", "Yes", "No" and so on.
const wchar_t* CGUISkin::getDefaultText(EGUI_DEFAULT_TEXT text)
{
    return Texts[text].c_str();
}

ox::video::ISpritePackage* CGUISkin::getSpritePackage()
{
    return SpritePackage;
}

//! sets a default font
void CGUISkin::setFont(IGUIFont* font)
{
    if (Font)
        Font->drop();

    Font = font;

    if (Font)
        Font->grab();
}

void CGUISkin::setSpritePackage(ox::video::ISpritePackage* package)
{
    SpritePackage = package;
    if (!SpritePackage)
        return;

    ox::video::ISpriteAnimationState* sprite = SpritePackage->addNewAnimationState("TextButtonNormal");
    if (sprite)
    {
        Sizes[EGDS_BUTTON_WIDTH] = sprite->getFrameSize(0).Width;
        Sizes[EGDS_BUTTON_HEIGHT] = sprite->getFrameSize(0).Height;
        sprite->remove();
    }

    sprite = SpritePackage->addNewAnimationState(L"VerScrollBackground");
    if (sprite)
    {
        Sizes[EGDS_SCROLLBAR_SIZE] = sprite->getFrameSize(0).Width;
        sprite->remove();
    }

    sprite = SpritePackage->addNewAnimationState(L"CheckboxNormal");
    if (sprite)
    {
        Sizes[EGDS_CHECK_BOX_WIDTH] = sprite->getFrameSize(0).Width;
        sprite->remove();
    }
}

//! destructor
CGUISkin::~CGUISkin()
{
    if (Font)
        Font->drop();
}

//! Sets a default text. For example for Message box button captions:
//! "OK", "Cancel", "Yes", "No" and so on.
void CGUISkin::setDefaultText(EGUI_DEFAULT_TEXT which, const wchar_t* newText)
{
    Texts[which] = newText;
}

CGUISkin::CGUISkin(EGUI_SKIN_TYPE type)
    : Font(0), SpritePackage(0)
{
    Colors[EGDC_3D_DARK_SHADOW] = ox::video::SColor(255, 50, 50, 50);
    Colors[EGDC_3D_SHADOW] = ox::video::SColor(255, 130, 130, 130);
    Colors[EGDC_3D_FACE] = ox::video::SColor(255, 210, 210, 210);
    Colors[EGDC_3D_HIGH_LIGHT] = ox::video::SColor(255, 255, 255, 255);
    Colors[EGDC_3D_LIGHT] = ox::video::SColor(255, 210, 210, 210);
    Colors[EGDC_ACTIVE_BORDER] = ox::video::SColor(255, 16, 14, 115);
    Colors[EGDC_ACTIVE_CAPTION] = ox::video::SColor(255, 255, 255, 255);
    Colors[EGDC_APP_WORKSPACE] = ox::video::SColor(255, 100, 100, 100);
    Colors[EGDC_BUTTON_TEXT] = ox::video::SColor(255, 0, 0, 0);
    Colors[EGDC_GRAY_TEXT] = ox::video::SColor(255, 130, 130, 130);
    Colors[EGDC_HIGH_LIGHT] = ox::video::SColor(255, 8, 36, 107);
    Colors[EGDC_HIGH_LIGHT_TEXT] = ox::video::SColor(255, 255, 255, 255);
    Colors[EGDC_INACTIVE_BORDER] = ox::video::SColor(255, 165, 165, 165);
    Colors[EGDC_INACTIVE_CAPTION] = ox::video::SColor(255, 210, 210, 210);
    Colors[EGDC_TOOLTIP] = ox::video::SColor(255, 255, 255, 230);
    Colors[EGDC_SCROLLBAR] = ox::video::SColor(255, 230, 230, 230);
    Colors[EGDC_WINDOW] = ox::video::SColor(255, 255, 255, 255);
    Colors[EGDC_MODAL_SCREEN] = ox::video::SColor(64, 0, 0, 0);
    Colors[EGDC_LIST_HIGH_LIGHT] = ox::video::SColor(48, 128, 128, 255);
    Colors[EGDC_LIST_HIGH_LIGHT_BORDER] = ox::video::SColor(128, 255, 255, 255);

    Sizes[EGDS_SCROLLBAR_SIZE] = 14;
    Sizes[EGDS_MENU_HEIGHT] = 18;
    Sizes[EGDS_WINDOW_BUTTON_WIDTH] = 15;
    Sizes[EGDS_CHECK_BOX_WIDTH] = 18;
    Sizes[EGDS_MESSAGE_BOX_WIDTH] = 500;
    Sizes[EGDS_MESSAGE_BOX_HEIGHT] = 200;
    Sizes[EGDS_BUTTON_WIDTH] = 80;
    Sizes[EGDS_BUTTON_HEIGHT] = 30;

    Texts[EGDT_MSG_BOX_OK] = L"OK";
    Texts[EGDT_MSG_BOX_CANCEL] = L"Cancel";
    Texts[EGDT_MSG_BOX_YES] = L"Yes";
    Texts[EGDT_MSG_BOX_NO] = L"No";
}

//! creates a color skin
IGUISkin* createSkin(EGUI_SKIN_TYPE type)
{
    return new CGUISkin(type);
}

} // end namespace gui
} // end namespace daisy
