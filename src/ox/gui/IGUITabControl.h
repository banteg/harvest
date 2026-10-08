// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUITabControl.

#ifndef OX_GUI_IGUITABCONTROL_H
#define OX_GUI_IGUITABCONTROL_H

#include "IGUILayout.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

//! A page of a tab control.
class IGUITab : public IGUILayout
{
public:
    IGUITab(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUILayout(environment, parent, id, rectangle)
    {
        Type = EGUIET_TAB;
    }

    //! Returns the number of this tab in its tab control.
    virtual int getNumber() = 0;
    virtual void setDrawBackground(bool draw) = 0;
    virtual void setBackgroundColor(video::SColor color) = 0;
};

//! A control with a row of tabs over their pages.
class IGUITabControl : public IGUIElement
{
public:
    IGUITabControl(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! The caption is not const in the Mac and Linux symbols.
    virtual IGUITab* addTab(wchar_t* caption, int id) = 0;
    virtual int getTabcount() = 0;
    virtual IGUITab* getTab(int index) = 0;
    virtual bool setActiveTab(int index) = 0;
    virtual int getActiveTab() = 0;
    virtual void setAnimations(video::ISpritePackage* package, const char* name) = 0;
    virtual void setTextColor(video::SColor color) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
