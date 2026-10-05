// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_GUI_CGUIPOPUPMENU_H
#define DAISY_GUI_CGUIPOPUPMENU_H

#include "ox/gui/IGUIPopupMenu.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/gui/IGUIFont.h"

namespace daisy {
namespace gui {

//! A popup menu: a frame with an optional title and one static text per option.
class CGUIPopupMenu : public ox::gui::IGUIPopupMenu
{
public:
    //! The menu covers its parent, so that it sees the clicks outside the frame.
    CGUIPopupMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CPosition2d<int> position, int width, const wchar_t* text, ox::gui::IGUIFont* titleFont,
        ox::gui::IGUIFont* font);

    ~CGUIPopupMenu();

    virtual void addMenuOption(const wchar_t* text, int id);
    virtual void setSelectionParent(ox::gui::IGUIElement* parent);

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and frames the selected option
    virtual void draw();

private:
    ox::gui::IGUILayout* Frame;
    //! The option under the mouse, counted without the title, or -1.
    int Selected;
    int Width;
    ox::gui::IGUIStaticText* Title;
    ox::gui::IGUIFont* Font;
    ox::gui::IGUIElement* SelectionParent;
};

} // end namespace gui
} // end namespace daisy

#endif
