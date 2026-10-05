// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIEnvironment.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIENVIRONMENT_H
#define DAISY_GUI_CGUIENVIRONMENT_H

#include <vector>
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIHoverParent.h"
#include "ox/core/CString.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
}

namespace daisy {
namespace gui {

//! The GUI root element and widget factory. Oxeye adds hover descriptions, layout groups and the
//! extra widgets, and makes the root a layout.
class CGUIEnvironment : public ox::gui::IGUIEnvironment, public ox::gui::IGUILayout
{
public:
    CGUIEnvironment(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver, ox::IOSOperator* op);
    virtual ~CGUIEnvironment();

    //! Removes all elements.
    virtual void clearElements();

    //! draws all gui elements
    virtual void drawAll();

    //! sets the focus to an element
    virtual void setFocus(ox::gui::IGUIElement* element);

    //! removes the focus from an element
    virtual void removeFocus(ox::gui::IGUIElement* element);

    //! Returns if the element has focus
    virtual bool hasFocus(ox::gui::IGUIElement* element);

    //! returns the current video driver
    virtual ox::video::IVideoDriver* getVideoDriver();

    //! posts an input event to the environment
    virtual bool postEventFromUser(ox::event::SEvent event);

    //! This sets a new event receiver for gui events.
    virtual void setUserEventReceiver(ox::event::IEventReceiver* receiver);

    //! returns the current gui skin
    virtual ox::gui::IGUISkin* getSkin();

    //! returns the font, loading a CUnicodeFont for .fnt files and a CGUIFont otherwise
    virtual ox::gui::IGUIFont* getFont(const char* filename);

    //! returns default font
    virtual ox::gui::IGUIFont* getBuiltInFont();

    virtual const ox::core::CPosition2d<int>& getMousePosition();

    //! Returns the root gui element.
    virtual ox::gui::IGUIElement* getRootGUIElement();

    //! Returns the element that holds the hover descriptions.
    virtual ox::gui::IGUIElement* getHoverParentElement()
    {
        return HoverParent;
    }

    virtual ox::gui::IGUIElement* addModalScreen();
    virtual ox::gui::IGUIButton* addButton(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent,
        int id, const wchar_t* text);
    virtual ox::gui::IGUIElement* addTextButton(const wchar_t* text, const char* sprite, char* hoverSprite,
        ox::gui::IGUIFont* font, ox::video::SColor color, ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUILayout* addWindow(const ox::core::CRect<int>& rectangle, bool modal, const wchar_t* text,
        ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUILayout* addFrame(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUILayout* addDetachableFrame(const ox::core::CRect<int>& rectangle,
        ox::gui::IGUIElement* parent, int id, unsigned int flags, bool detached, bool visible);
    virtual ox::gui::IGUILayout* addLayoutGroup(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent);
    virtual ox::gui::IGUILayout* addHoverDescription(ox::gui::IGUIElement* element, const wchar_t* text,
        ox::video::SColor* color);
    virtual ox::gui::IGUIElement* addMessageBox(const wchar_t* caption, const wchar_t* text, bool modal, int flags,
        ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUIScrollBar* addScrollBar(bool horizontal, const ox::core::CRect<int>& rectangle,
        ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUIImage* addImage(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent, int id,
        const wchar_t* text);
    virtual ox::gui::IGUICheckBox* addCheckBox(bool checked, const ox::core::CRect<int>& rectangle,
        ox::gui::IGUIElement* parent, int id, const wchar_t* text);
    virtual ox::gui::IGUIRadioList* addRadioList(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent,
        int id);
    virtual ox::gui::IGUIListBox* addListBox(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent,
        int id, bool drawBackground);
    virtual ox::gui::IGUIElement* addMeshViewer(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent,
        int id, const wchar_t* text);
    virtual ox::gui::IGUIFileOpenDialog* addFileOpenDialog(const wchar_t* title, bool modal,
        ox::gui::IGUIElement* parent, int id, const char* directory, const char* filter);
    virtual ox::gui::IGUIStaticText* addStaticText(const wchar_t* text, const ox::core::CRect<int>& rectangle,
        bool border, bool wordWrap, ox::gui::IGUIElement* parent, int id, const wchar_t* style);
    //! A static text of the given width and the height its broken lines need.
    virtual ox::gui::IGUIStaticText* addStaticText(const wchar_t* text, int width, ox::gui::IGUIElement* parent,
        ox::gui::IGUIFont* font, int id, const wchar_t* style);
    //! A static text placed by a layout's sorters according to the layout hints.
    virtual ox::gui::IGUIStaticText* addStaticText(const wchar_t* text, const char* layout,
        ox::gui::IGUIElement* parent, ox::gui::IGUIFont* font, int id);
    virtual ox::gui::IGUIEditBox* addEditBox(const wchar_t* text, const ox::core::CRect<int>& rectangle, bool border,
        ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUIElement* addInOutFader(const ox::core::CRect<int>* rectangle, ox::gui::IGUIElement* parent,
        int id);
    virtual ox::gui::IGUITabControl* addTabControl(const ox::core::CRect<int>& rectangle,
        ox::gui::IGUIElement* parent, bool background, bool border, int id);
    virtual ox::gui::IGUITabButtonRow* addTabButtonRow(const ox::core::CPosition2d<int>& position, int width,
        ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUITab* addTab(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUIContextMenu* addContextMenu(const ox::core::CRect<int>& rectangle,
        ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUIContextMenu* addMenu(ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUIToolBar* addToolBar(ox::gui::IGUIElement* parent, int id);
    virtual ox::gui::IGUIComboBox* addComboBox(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent,
        int id);
    virtual ox::gui::IGUIPopupMenu* addPopupMenu(const ox::core::CPosition2d<int>& position, int width,
        const wchar_t* text, ox::gui::IGUIFont* font, ox::gui::IGUIFont* hoverFont, ox::gui::IGUIElement* parent,
        int id);
    virtual ox::gui::IGUIElement* addClickArea(const ox::core::CRect<int>& rectangle, ox::gui::IGUIElement* parent,
        int id);

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! Adding an element drops the hover and the focus.
    virtual void addChild(ox::gui::IGUIElement* child);

    //! Removing an element drops the hover and the focus.
    virtual void removeChild(ox::gui::IGUIElement* child)
    {
        if (Hovered)
        {
            Hovered->drop();
            Hovered = 0;
        }

        if (Focus)
        {
            Focus->drop();
            Focus = 0;
        }

        IGUIElement::removeChild(child);
    }

private:
    struct SFont
    {
        ox::core::CString<char> Filename;
        ox::gui::IGUIFont* Font;

        bool operator<(const SFont& other) const
        {
            return Filename < other.Filename;
        }
    };

    void updateHoveredElement(ox::core::CPosition2d<int> mousePos);

    void loadBuidInFont();

    //! A pointer-sized member that neither build's environment code touches.
    void* Unused;
    std::vector<SFont> Fonts;
    ox::video::IVideoDriver* Driver;
    ox::gui::IGUIElement* Hovered;
    ox::gui::IGUIElement* Focus;
    ox::gui::IGUISkin* CurrentSkin;
    ox::io::IFileSystem* FileSystem;
    ox::event::IEventReceiver* UserReceiver;
    ox::IOSOperator* Operator;
    ox::core::CPosition2d<int> MousePosition;
    ox::gui::IGUIHoverParent* HoverParent;
};

//! creates a GUI environment
ox::gui::IGUIEnvironment* createGUIEnvironment(ox::io::IFileSystem* fs, ox::video::IVideoDriver* driver,
    ox::IOSOperator* op);

} // end namespace gui
} // end namespace daisy

#endif
