// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIScrollBar.

#ifndef OX_GUI_IGUISCROLLBAR_H
#define OX_GUI_IGUISCROLLBAR_H

#include "IGUIElement.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

//! A scroll bar, also used as a slider.
class IGUIScrollBar : public IGUIElement
{
public:
    IGUIScrollBar(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    virtual void setMax(int max) = 0;
    virtual int getMax() = 0;
    virtual void setStepSizes(int smallStep, int largeStep) = 0;
    virtual int getPos() = 0;
    virtual void setPos(int pos) = 0;
    virtual bool isDragging() = 0;
    virtual void setAnimations(video::ISpritePackage* package, const char* name) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
