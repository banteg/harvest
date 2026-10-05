// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The methods are inline; their copies are emitted in daisy/gui/CGUIEnvironment.cpp.

#ifndef OX_GUI_IGUIHOVERPARENT_H
#define OX_GUI_IGUIHOVERPARENT_H

#include "IGUIElement.h"
#include "IGUIElementInline.h"
#include "IGUIEnvironment.h"

namespace ox {
namespace gui {

//! The screen-sized element that holds the hover descriptions. It does not own its children's
//! parent links, and passes its events on to the root element.
class IGUIHoverParent : public IGUIElement
{
public:
    IGUIHoverParent(IGUIEnvironment* environment, core::CRect<int> rectangle)
        : IGUIElement(environment, 0, -1, rectangle)
    {
        EventReceiver = environment->getRootGUIElement();
    }

    virtual ~IGUIHoverParent()
    {
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            (*it)->Parent = 0;
        Children.clear();
    }

    //! Hides every hover description except item.
    void displayHover(IGUIElement* item)
    {
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            (*it)->setVisible(false);

        if (item)
            item->setVisible(true);
    }
};

} // end namespace gui
} // end namespace ox

#endif
