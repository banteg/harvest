// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The inline IGUIElement members that the element implementations inline (the constructor in every
// widget constructor, setText in CGUIMessageBox's). They are kept out of IGUIElement.h because
// instantiating their CString members there changes GCC's code for game units that include it
// (CSaveGameScreen's sort helpers).

#ifndef OX_GUI_IGUIELEMENTINLINE_H
#define OX_GUI_IGUIELEMENTINLINE_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

inline IGUIElement::IGUIElement(IGUIEnvironment* environment, IGUIElement* parent, int id,
    core::CRect<int> rectangle)
    : Parent(parent), RelativeRect(rectangle), RelativeSizeChanged(false), AbsoluteRect(0, 0, 0, 0),
      AbsoluteClippingRect(0, 0, 0, 0), IsVisible(true), IsEnabled(true), IsFixed(false), IsInvisible(false),
      NoClip(false), ReportOnDraw(0), ID(id), Type(0), Environment(environment), HoverItem(0), LayoutFlags(0),
      EventReceiver(0)
{
    AbsoluteRect = RelativeRect;
    AbsoluteClippingRect = AbsoluteRect;
    updateAbsolutePosition();

    if (Parent)
        Parent->addChild(this);
}

inline void IGUIElement::setText(const wchar_t* text)
{
    Text = text;
}

} // end namespace gui
} // end namespace ox

#endif
