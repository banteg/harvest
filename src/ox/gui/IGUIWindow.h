// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIWindow.

#ifndef OX_GUI_IGUIWINDOW_H
#define OX_GUI_IGUIWINDOW_H

#include "IGUILayout.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIButton;

//! A layout with a frame, as made by IGUIEnvironment::addWindow and addFrame.
class IGUIWindow : public IGUILayout
{
public:
    // Defined inline in IGUILayoutInline.h.
    IGUIWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle);
    virtual IGUIButton* getCloseButton() = 0;
    virtual IGUIButton* getMinimizeButton() = 0;
    virtual IGUIButton* getMaximizeButton() = 0;
    //! Draws the frame with the named animation of the sprite package.
    virtual void setAnimations(video::ISpritePackage* package, const char* animation) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
