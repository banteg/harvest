// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIDetachableFrame.

#ifndef OX_GUI_IGUIDETACHABLEFRAME_H
#define OX_GUI_IGUIDETACHABLEFRAME_H

#include "IGUILayout.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIFont;

//! A frame that the user can drag, resize, lock and fade, with an options menu, as made by
//! IGUIEnvironment::addDetachableFrame.
class IGUIDetachableFrame : public IGUILayout
{
public:
    IGUIDetachableFrame(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUILayout(environment, parent, id, rectangle)
    {
        Type = EGUIET_DETACHABLE_FRAME;
    }

    //! Draws the frame with the named animation of the sprite package.
    virtual void setAnimations(video::ISpritePackage* package, const char* animation) = 0;
    virtual void setPopupMenuTitleFont(IGUIFont* font) = 0;
    virtual void setPopupMenuItemsFont(IGUIFont* font) = 0;
    //! Adds an entry to the options menu; choosing it sends a popup menu event with the id.
    virtual void addMenuOption(const wchar_t* text, int id) = 0;
    virtual void setMenuVisible(bool visible) = 0;
    virtual void setResizable(bool resizable, const core::CDimension2d<int>& minimumSize) = 0;
    virtual void setLocked(bool locked) = 0;
    virtual void setFadeable(bool fadeable) = 0;
    virtual void setLockable(bool lockable) = 0;
    virtual void setHideable(bool hideable) = 0;
    virtual void stopDragging() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
