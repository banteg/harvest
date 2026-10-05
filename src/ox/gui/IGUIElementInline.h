// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Inline bodies of IGUIElement that the element implementations inline. They live apart from
// IGUIElement.h because more inline functions there change the code GCC generates for game units
// that include it (CSaveGameScreen::loadSaveGames).

#ifndef OX_GUI_IGUIELEMENTINLINE_H
#define OX_GUI_IGUIELEMENTINLINE_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

inline IGUIElement::IGUIElement(IGUIEnvironment* environment, IGUIElement* parent, int id,
    core::CRect<int> rectangle)
    : Parent(parent), RelativeRect(rectangle), RelativeSizeChanged(false), IsVisible(true), IsEnabled(true),
      IsFixed(false), IsInvisible(false), NoClip(false), ReportOnDraw(0), ID(id), Type(0),
      Environment(environment), HoverItem(0), LayoutFlags(0), EventReceiver(0)
{
    AbsoluteRect = RelativeRect;
    AbsoluteClippingRect = RelativeRect;
    updateAbsolutePosition();

    if (Parent)
        Parent->addChild(this);
}

} // end namespace gui
} // end namespace ox

#endif
