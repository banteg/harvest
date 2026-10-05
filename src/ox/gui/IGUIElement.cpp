// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "IGUIElementInline.h"
#include "IGUIEnvironment.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace gui {

void IGUIElement::remove()
{
    if (Environment->hasFocus(this))
        Environment->removeFocus(this);

    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        (*it)->Parent = 0;
        (*it)->remove();
        (*it)->drop();
    }
    Children.clear();

    if (Parent)
        Parent->removeChild(this);
}

} // end namespace gui
} // end namespace ox
