// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIEnvironment.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye keeps the fonts sorted in a std::vector, loads .fnt files as CUnicodeFont, tracks the mouse
// position, shows hover descriptions above the hovered element and adds the extra widgets.

#include "CGUIEnvironment.h"
#include "ox/IOSOperator.h"
#include "ox/algo/CArrayFunctions.h"
#include "ox/core/CStringFunctions.h"
#include "ox/gui/IGUIHoverItem.h"
#include "ox/gui/IGUISkin.h"
#include "ox/io/IFileSystem.h"
#include "ox/video/IVideoDriver.h"
#include "daisy/io/CMemoryReadFile.h"
#include "daisy/os.h"
#include "BuildInFont.h"
#include "CGUIClickArea.h"
#include "CGUIButton.h"
#include "CGUICheckBox.h"
#include "CGUIComboBox.h"
#include "CGUIContextMenu.h"
#include "CGUIDetachableFrame.h"
#include "CGUIEditBox.h"
#include "CGUIFileOpenDialog.h"
#include "CGUIFont.h"
#include "CGUIImage.h"
#include "CGUIInOutFader.h"
#include "CGUIListBox.h"
#include "CGUIMenu.h"
#include "CGUIMeshViewer.h"
#include "CGUIMessageBox.h"
#include "CGUIModalScreen.h"
#include "CGUIPopupMenu.h"
#include "CGUIRadioList.h"
#include "CGUIScrollBar.h"
#include "CGUISkin.h"
#include "CGUIStaticText.h"
#include "CGUITabButtonRow.h"
#include "CGUITabControl.h"
#include "CGUIToolBar.h"
#include "CGUIWindow.h"
#include "CUnicodeFont.h"
#include <iostream>

namespace daisy {
namespace gui {

using namespace ox::gui;
using ox::core::CRect;
using ox::core::CPosition2d;
using ox::core::CDimension2d;
using ox::event::SEvent;

//! constructor
CGUIEnvironment::CGUIEnvironment(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver, ox::IOSOperator* op)
    : IGUILayout(0, 0, 0, CRect<int>(CPosition2d<int>(0, 0), driver ? driver->getScreenSize() : CDimension2d<int>(0,
        0))),
      Driver(driver), Hovered(0), Focus(0), CurrentSkin(0), FileSystem(fs), UserReceiver(0), Operator(op),
      MousePosition(0, 0)
{
    if (Driver)
        Driver->grab();

    if (FileSystem)
        FileSystem->grab();

    if (Operator)
        Operator->grab();

    loadBuidInFont();

    CurrentSkin = createSkin(EGST_WINDOWS_STANDARD);
    CurrentSkin->setFont(getBuiltInFont());

    HoverParent = new IGUIHoverParent(this, AbsoluteRect);
}

void CGUIEnvironment::loadBuidInFont()
{
    const char* filename = "#DefaultFont";

    ox::io::IReadFile* file = daisy::io::createMemoryReadFile(BuildInFontData, BuildInFontDataSize, filename, false);

    CGUIFont* font = new CGUIFont(Driver);
    if (!font->load(file))
    {
        daisy::os::Printer::log("Error: Could not load built-in Font.", ox::event::ELL_ERROR);
        font->drop();
        file->drop();
        return;
    }

    SFont f;
    f.Filename = filename;
    f.Font = font;
    Fonts.push_back(f);
    ox::algo::sort(Fonts.begin(), Fonts.end());

    file->drop();
}

//! destructor
CGUIEnvironment::~CGUIEnvironment()
{
    if (Hovered)
        Hovered->drop();

    if (HoverParent)
        HoverParent->drop();

    if (CurrentSkin)
        CurrentSkin->drop();

    if (Driver)
        Driver->drop();

    if (Focus)
        Focus->drop();

    if (FileSystem)
        FileSystem->drop();

    if (Operator)
        Operator->drop();

    // delete all fonts
    for (unsigned int i = 0; i < Fonts.size(); ++i)
        Fonts[i].Font->drop();
}

//! Removes all elements.
void CGUIEnvironment::clearElements()
{
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        (*it)->drop();
    Children.clear();
}

//! draws all gui elements
void CGUIEnvironment::drawAll()
{
    if (Driver)
    {
        CDimension2d<int> dim = Driver->getScreenSize();
        if (AbsoluteRect.LowerRightCorner.X != dim.Width || AbsoluteRect.LowerRightCorner.Y != dim.Height)
        {
            // resize gui environment
            RelativeRect.LowerRightCorner.X = dim.Width;
            RelativeRect.LowerRightCorner.Y = dim.Height;
            AbsoluteClippingRect = RelativeRect;
            AbsoluteRect = RelativeRect;
            updateAbsolutePosition();

            if (HoverParent)
                HoverParent->setRelativePosition(RelativeRect);
        }
    }

    draw();

    if (HoverParent && Hovered && Hovered->getHoverItem())
    {
        IGUIHoverItem::placeHoverItem(Hovered, Hovered->getHoverItem());
        HoverParent->draw();
    }
}

//! sets the focus to an element
void CGUIEnvironment::setFocus(IGUIElement* element)
{
    if (Focus == element)
        return;

    removeFocus(Focus);

    Focus = element;
    if (Focus)
        Focus->grab();
}

//! removes the focus from an element
void CGUIEnvironment::removeFocus(IGUIElement* element)
{
    if (Focus && Focus == element)
    {
        SEvent e;
        e.EventType = ox::event::EET_GUI_EVENT;
        e.GUIEvent.Caller = this;
        e.GUIEvent.EventType = ox::gui::EGET_ELEMENT_FOCUS_LOST;
        Focus = 0;
        element->OnEvent(e);
        element->drop();
    }
}

//! Returns if the element has focus
bool CGUIEnvironment::hasFocus(IGUIElement* element)
{
    return element == Focus;
}

//! returns the current video driver
ox::video::IVideoDriver* CGUIEnvironment::getVideoDriver()
{
    return Driver;
}

//! called by ui if an event happened.
bool CGUIEnvironment::OnEvent(const SEvent& event)
{
    if (UserReceiver && event.GUIEvent.Caller != this && event.EventType == ox::event::EET_GUI_EVENT)
        return UserReceiver->OnEvent(event);

    return false;
}

void CGUIEnvironment::updateHoveredElement(CPosition2d<int> mousePos)
{
    IGUIElement* lastHovered = Hovered;

    Hovered = getElementFromPoint(mousePos);

    if (Hovered)
    {
        Hovered->grab();

        if (Hovered != lastHovered)
        {
            SEvent event;
            event.EventType = ox::event::EET_GUI_EVENT;

            if (lastHovered)
            {
                event.GUIEvent.Caller = lastHovered;
                event.GUIEvent.EventType = EGET_ELEMENT_LEFT;
                lastHovered->OnEvent(event);

                if (lastHovered->getType() == EGUIET_BUTTON)
                {
                    event.GUIEvent.EventType = EGET_BUTTON_LEFT;
                    lastHovered->OnEvent(event);
                }
            }

            event.GUIEvent.Caller = Hovered;
            event.GUIEvent.EventType = EGET_ELEMENT_HOVERED;
            Hovered->OnEvent(event);

            if (Hovered->getType() == EGUIET_BUTTON)
            {
                event.GUIEvent.EventType = EGET_BUTTON_HOVERED;
                Hovered->OnEvent(event);
            }

            HoverParent->displayHover(Hovered->getHoverItem());
        }
    }

    if (lastHovered)
        lastHovered->drop();
}

//! This sets a new event receiver for gui events. Usually you do not have to
//! use this method, it is used by the internal engine.
void CGUIEnvironment::setUserEventReceiver(ox::event::IEventReceiver* receiver)
{
    UserReceiver = receiver;
}

//! posts an input event to the environment
bool CGUIEnvironment::postEventFromUser(SEvent event)
{
    switch (event.EventType)
    {
    case ox::event::EET_MOUSE_INPUT_EVENT:
        // the wheel event carries no position
        if (event.MouseInput.Event == ox::event::EMIE_MOUSE_WHEEL)
        {
            event.MouseInput.X = MousePosition.X;
            event.MouseInput.Y = MousePosition.Y;
        }
        else
        {
            MousePosition.X = event.MouseInput.X;
            MousePosition.Y = event.MouseInput.Y;
        }

        // a button released outside the focused element takes its focus
        if (Focus)
        {
            if (event.MouseInput.Event >= ox::event::EMIE_LMOUSE_LEFT_UP
                && event.MouseInput.Event <= ox::event::EMIE_MMOUSE_LEFT_UP
                && !Focus->getAbsolutePosition().isPointInside(MousePosition))
                removeFocus(Focus);
            else if (Focus->OnEvent(event))
                return true;
        }

        updateHoveredElement(MousePosition);

        if (Hovered && Hovered != this)
            return Hovered->OnEvent(event);
        break;
    case ox::event::EET_KEY_INPUT_EVENT:
        if (Focus && Focus != this && Focus->OnEvent(event))
            return true;
        return OnEventInNonFocusState(event);
    default:
        break;
    }

    return false;
}

//! returns the current gui skin
IGUISkin* CGUIEnvironment::getSkin()
{
    return CurrentSkin;
}

//! Adding an element drops the hover, hiding its description, and the focus.
void CGUIEnvironment::addChild(IGUIElement* child)
{
    if (Hovered && Hovered->getHoverItem())
    {
        Hovered->getHoverItem()->setVisible(false);
        Hovered->drop();
        Hovered = 0;
    }

    if (Focus)
    {
        Focus->drop();
        Focus = 0;
    }

    IGUIElement::addChild(child);
}

IGUIElement* CGUIEnvironment::addModalScreen()
{
    IGUIElement* screen = new CGUIModalScreen(this, this, -1);
    screen->drop();
    return screen;
}

//! adds an button. The returned pointer must not be dropped.
IGUIButton* CGUIEnvironment::addButton(const CRect<int>& rectangle, IGUIElement* parent, int id, const wchar_t* text)
{
    IGUIButton* button = new CGUIButton(this, parent ? parent : this, id, rectangle, false, text);
    button->drop();
    return button;
}

IGUIElement* CGUIEnvironment::addTextButton(const wchar_t* text, const char* sprite, char* hoverSprite,
    IGUIFont* font, ox::video::SColor color, IGUIElement* parent, int id)
{
    IGUIElement* button = new CGUITextButton(this, parent ? parent : this, text, id,
        font ? font : CurrentSkin->getFont(), color, sprite, hoverSprite);
    button->drop();
    return button;
}

//! adds a window. The returned pointer must not be dropped.
IGUILayout* CGUIEnvironment::addWindow(const CRect<int>& rectangle, bool modal, const wchar_t* text,
    IGUIElement* parent, int id)
{
    parent = parent ? parent : this;

    if (modal)
    {
        parent = new CGUIModalScreen(this, parent, -1);
        parent->drop();
    }

    IGUIWindow* win = new CGUIWindow(this, parent, id, rectangle, false);
    if (text)
        win->setText(text);
    win->drop();

    return win;
}

//! adds a window frame without a title bar.
IGUILayout* CGUIEnvironment::addFrame(const CRect<int>& rectangle, IGUIElement* parent, int id)
{
    parent = parent ? parent : this;

    IGUIWindow* win = new CGUIWindow(this, parent, id, rectangle, true);
    win->drop();
    return win;
}

const CPosition2d<int>& CGUIEnvironment::getMousePosition()
{
    return MousePosition;
}

IGUILayout* CGUIEnvironment::addDetachableFrame(const CRect<int>& rectangle, IGUIElement* parent, int id,
    unsigned int flags, bool detached, bool visible)
{
    parent = parent ? parent : this;

    IGUILayout* frame = new CGUIDetachableFrame(this, parent, id, rectangle, flags, detached, visible);
    frame->drop();
    return frame;
}

IGUILayout* CGUIEnvironment::addLayoutGroup(const CRect<int>& rectangle, IGUIElement* parent)
{
    parent = parent ? parent : this;

    IGUILayout* group = new IGUILayout(this, parent, -1, rectangle);
    group->drop();
    group->setInvisible(true);
    return group;
}

IGUILayout* CGUIEnvironment::addHoverDescription(IGUIElement* element, const wchar_t* text, ox::video::SColor* color)
{
    IGUILayout* group = 0;
    if (element)
    {
        ox::video::SColor textColor = CurrentSkin->getColor(EGDC_HIGH_LIGHT_TEXT);
        if (color)
            textColor = *color;

        CDimension2d<int> size = CurrentSkin->getFont()->getDimension(text);
        group = addFrame(CRect<int>(0, 0, size.Width + 2, size.Height + 2), HoverParent, -1);
        IGUIStaticText* description = addStaticText(text, CRect<int>(0, 0, size.Width, size.Height), false, false,
            group, -1, 0);
        description->setOverrideColor(textColor);
        group->sortVertically(1, true);
        element->setHoverItem(group);
    }

    return group;
}

//! Adds a message box.
IGUIElement* CGUIEnvironment::addMessageBox(const wchar_t* caption, const wchar_t* text, bool modal, int flags,
    IGUIElement* parent, int id)
{
    if (!CurrentSkin)
        return 0;

    parent = parent ? parent : this;

    CRect<int> rect;
    CDimension2d<int> screenDim, msgBoxDim;

    screenDim.Width = parent->getAbsolutePosition().getWidth();
    screenDim.Height = parent->getAbsolutePosition().getHeight();
    msgBoxDim.Width = CurrentSkin->getSize(EGDS_MESSAGE_BOX_WIDTH);
    msgBoxDim.Height = CurrentSkin->getSize(EGDS_MESSAGE_BOX_HEIGHT);

    rect.UpperLeftCorner.X = (screenDim.Width - msgBoxDim.Width) / 2;
    rect.UpperLeftCorner.Y = (screenDim.Height - msgBoxDim.Height) / 2;
    rect.LowerRightCorner.X = rect.UpperLeftCorner.X + msgBoxDim.Width;
    rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + msgBoxDim.Height;

    if (modal)
    {
        parent = new CGUIModalScreen(this, parent, -1);
        parent->drop();
    }

    IGUIElement* win = new CGUIMessageBox(this, caption, text, flags, parent, id, rect);
    win->drop();
    return win;
}

//! adds a scrollbar. The returned pointer must not be dropped.
IGUIScrollBar* CGUIEnvironment::addScrollBar(bool horizontal, const CRect<int>& rectangle, IGUIElement* parent,
    int id)
{
    IGUIScrollBar* bar = new CGUIScrollBar(horizontal, this, parent ? parent : this, id, rectangle, false);
    bar->drop();
    return bar;
}

//! adds an image. The returned pointer must not be dropped.
IGUIImage* CGUIEnvironment::addImage(const CRect<int>& rectangle, IGUIElement* parent, int id, const wchar_t* text)
{
    IGUIImage* img = new CGUIImage(this, parent ? parent : this, id, rectangle);

    if (text)
        img->setText(text);

    img->drop();
    return img;
}

//! adds an mesh viewer. The returned pointer must not be dropped.
IGUIElement* CGUIEnvironment::addMeshViewer(const CRect<int>& rectangle, IGUIElement* parent, int id,
    const wchar_t* text)
{
    IGUIElement* v = new CGUIMeshViewer(this, parent ? parent : this, id, rectangle);

    if (text)
        v->setText(text);

    v->drop();
    return v;
}

//! adds a checkbox
IGUICheckBox* CGUIEnvironment::addCheckBox(bool checked, const CRect<int>& rectangle, IGUIElement* parent, int id,
    const wchar_t* text)
{
    IGUICheckBox* b = new CGUICheckBox(checked, this, parent ? parent : this, id, rectangle);

    if (text)
        b->setText(text);

    b->drop();
    return b;
}

IGUIRadioList* CGUIEnvironment::addRadioList(const CRect<int>& rectangle, IGUIElement* parent, int id)
{
    IGUIRadioList* list = new CGUIRadioList(this, parent ? parent : this, rectangle, id);
    list->drop();
    return list;
}

//! adds a list box
IGUIListBox* CGUIEnvironment::addListBox(const CRect<int>& rectangle, IGUIElement* parent, int id,
    bool drawBackground)
{
    IGUIListBox* b = new CGUIListBox(this, parent ? parent : this, id, rectangle, true, drawBackground, false);
    b->drop();
    return b;
}

//! adds a file open dialog. The returned pointer must not be dropped.
IGUIFileOpenDialog* CGUIEnvironment::addFileOpenDialog(const wchar_t* title, bool modal, IGUIElement* parent, int id,
    const char* directory, const char* filter)
{
    parent = parent ? parent : this;

    if (modal)
    {
        parent = new CGUIModalScreen(this, parent, -1);
        parent->drop();
    }

    IGUIFileOpenDialog* d = new CGUIFileOpenDialog(FileSystem, title, this, parent, id, directory, filter);
    d->drop();
    return d;
}

//! adds a static text. The returned pointer must not be dropped.
IGUIStaticText* CGUIEnvironment::addStaticText(const wchar_t* text, const CRect<int>& rectangle, bool border,
    bool wordWrap, IGUIElement* parent, int id, const wchar_t* style)
{
    IGUIStaticText* d = new CGUIStaticText(text, border, this, parent ? parent : this, id, rectangle, style);
    d->setWordWrap(wordWrap);
    d->drop();
    return d;
}

IGUIStaticText* CGUIEnvironment::addStaticText(const wchar_t* text, int width, IGUIElement* parent, IGUIFont* font,
    int id, const wchar_t* style)
{
    if (!font)
    {
        font = CurrentSkin->getFont();
        if (!font)
            return 0;
    }

    IGUIElement* textParent = parent ? parent : this;
    const wchar_t* textStyle = style ? style : L"";

    int height = IGUIStaticText::getMultilineHeight(ox::core::CString<wchar_t>(text), font, width, textStyle);
    IGUIStaticText* staticText = addStaticText(text, CRect<int>(0, 0, width, height), false, true, textParent, id,
        textStyle);
    staticText->setOverrideFont(font);
    return staticText;
}

IGUIStaticText* CGUIEnvironment::addStaticText(const wchar_t* text, const char* layout, IGUIElement* parent,
    IGUIFont* font, int id)
{
    if (!font)
    {
        font = CurrentSkin->getFont();
        if (!font)
            return 0;
    }

    IGUIElement* textParent = parent ? parent : this;
    IGUIStaticText* staticText = addStaticText(text, CRect<int>(0, 0, 10, 15), false, false, textParent, id, 0);
    staticText->LayoutFlags = layout;
    staticText->setOverrideFont(font);
    return staticText;
}

//! Adds an edit box. The returned pointer must not be dropped.
IGUIEditBox* CGUIEnvironment::addEditBox(const wchar_t* text, const CRect<int>& rectangle, bool border,
    IGUIElement* parent, int id)
{
    IGUIEditBox* d = new CGUIEditBox(text, border, this, parent ? parent : this, id, rectangle, Operator);
    d->drop();
    return d;
}

//! Adds a tab control to the environment.
IGUITabControl* CGUIEnvironment::addTabControl(const CRect<int>& rectangle, IGUIElement* parent, bool background,
    bool border, int id)
{
    IGUITabControl* t = new CGUITabControl(this, parent ? parent : this, rectangle, background, border, id);
    t->drop();
    return t;
}

IGUITabButtonRow* CGUIEnvironment::addTabButtonRow(const CPosition2d<int>& position, int width, IGUIElement* parent,
    int id)
{
    IGUITabButtonRow* row = new CGUITabButtonRow(this, parent ? parent : this,
        CRect<int>(position, CDimension2d<int>(width, 20)), id);
    row->drop();
    return row;
}

//! Adds tab to the environment.
IGUITab* CGUIEnvironment::addTab(const CRect<int>& rectangle, IGUIElement* parent, int id)
{
    IGUITab* t = new CGUITab(-1, this, parent ? parent : this, rectangle, id);
    t->drop();
    return t;
}

//! Adds a context menu to the environment.
IGUIContextMenu* CGUIEnvironment::addContextMenu(const CRect<int>& rectangle, IGUIElement* parent, int id)
{
    IGUIContextMenu* c = new CGUIContextMenu(this, parent ? parent : this, id, rectangle, true);
    c->drop();
    return c;
}

//! Adds a menu to the environment.
IGUIContextMenu* CGUIEnvironment::addMenu(IGUIElement* parent, int id)
{
    if (!parent)
        parent = this;

    IGUIContextMenu* c = new CGUIMenu(this, parent, id, CRect<int>(0, 0, parent->getAbsolutePosition().getWidth(),
        parent->getAbsolutePosition().getHeight()));
    c->drop();
    return c;
}

//! Adds a toolbar to the environment. It is like a menu is always placed on top
//! in its parent, and contains buttons.
IGUIToolBar* CGUIEnvironment::addToolBar(IGUIElement* parent, int id)
{
    if (!parent)
        parent = this;

    IGUIToolBar* b = new CGUIToolBar(this, parent, id, CRect<int>(0, 0, 10, 10));
    b->drop();
    return b;
}

//! Adds an element for fading in or out.
IGUIElement* CGUIEnvironment::addInOutFader(const CRect<int>* rectangle, IGUIElement* parent, int id)
{
    CRect<int> rect;

    if (rectangle)
        rect = *rectangle;
    else if (Driver)
        rect = CRect<int>(CPosition2d<int>(0, 0), Driver->getScreenSize());

    if (!parent)
        parent = this;

    IGUIElement* fader = new CGUIInOutFader(this, parent, id, rect);
    fader->drop();
    return fader;
}

//! Adds a combo box to the environment.
IGUIComboBox* CGUIEnvironment::addComboBox(const CRect<int>& rectangle, IGUIElement* parent, int id)
{
    IGUIComboBox* t = new CGUIComboBox(this, parent ? parent : this, id, rectangle);
    t->drop();
    return t;
}

IGUIElement* CGUIEnvironment::addClickArea(const CRect<int>& rectangle, IGUIElement* parent, int id)
{
    CClickArea* area = new CClickArea(this, parent ? parent : this, id, rectangle);
    area->drop();
    return area;
}

IGUIPopupMenu* CGUIEnvironment::addPopupMenu(const CPosition2d<int>& position, int width, const wchar_t* text,
    IGUIFont* font, IGUIFont* hoverFont, IGUIElement* parent, int id)
{
    CGUIPopupMenu* menu = new CGUIPopupMenu(this, parent ? parent : this, id, position, width, text,
        font ? font : CurrentSkin->getFont(), hoverFont ? hoverFont : CurrentSkin->getFont());
    menu->drop();
    return menu;
}

//! returns the font
IGUIFont* CGUIEnvironment::getFont(const char* filename)
{
    // search existing font
    SFont f;
    if (!filename)
        filename = "";

    f.Filename = filename;
    ox::core::CStringFunctions::ansiMakeLower(f.Filename);

    int index = ox::algo::binarySearchPos(Fonts.begin(), Fonts.end(), f);
    if (index != -1)
        return Fonts[index].Font;

    // not existing yet. try to load font.
    IGUIFont* font;
    ox::core::CString<char> name = filename;
    if (name.findNext(".fnt", 0) == -1)
    {
        CGUIFont* bitmapFont = new CGUIFont(Driver);
        if (!bitmapFont->load(filename))
        {
            bitmapFont->drop();
            return 0;
        }
        font = bitmapFont;
    }
    else
    {
        CUnicodeFont* unicodeFont = new CUnicodeFont(Driver);
        if (!unicodeFont->load(filename))
        {
            unicodeFont->drop();
            return 0;
        }
        font = unicodeFont;
    }

    // add to fonts.
    f.Font = font;
    Fonts.push_back(f);
    ox::algo::sort(Fonts.begin(), Fonts.end());

    return font;
}

//! returns default font
IGUIFont* CGUIEnvironment::getBuiltInFont()
{
    if (Fonts.empty())
        return 0;

    return Fonts[0].Font;
}

//! Returns the root gui element.
IGUIElement* CGUIEnvironment::getRootGUIElement()
{
    return this;
}

//! creates an GUI Environment
IGUIEnvironment* createGUIEnvironment(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver, ox::IOSOperator* op)
{
    return new CGUIEnvironment(fs, driver, op);
}

} // end namespace gui
} // end namespace daisy
