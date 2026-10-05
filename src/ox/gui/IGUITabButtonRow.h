// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUITabButtonRow.

#ifndef OX_GUI_IGUITABBUTTONROW_H
#define OX_GUI_IGUITABBUTTONROW_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIFont;

//! A row of tab buttons without pages; the active tab can have a close button.
class IGUITabButtonRow : public IGUIElement
{
public:
    IGUITabButtonRow(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! Adds a tab and returns its index.
    virtual int addTab(const wchar_t* caption, bool closable) = 0;
    virtual int getTabCount() = 0;
    virtual const wchar_t* getTabCaption(int index) = 0;
    virtual void setActiveTabButton(int index) = 0;
    //! Draws the caption of the tab in the highlight color.
    virtual void setTabButtonHighlight(int index, bool highlight) = 0;
    virtual int getActiveTabButton() = 0;
    virtual void setAnimations(video::ISpritePackage* package, const char* name) = 0;
    virtual void setTextColor(video::SColor color) = 0;
    virtual void setTextFont(IGUIFont* font) = 0;
    virtual void setHighlightColor(video::SColor color) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
