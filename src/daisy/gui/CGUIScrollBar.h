// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only what CGUIListBox uses. The owner of the CGUIScrollBar unit replaces this header.

#ifndef DAISY_GUI_CGUISCROLLBAR_H
#define DAISY_GUI_CGUISCROLLBAR_H

#include "ox/gui/IGUIScrollBar.h"

namespace daisy {
namespace gui {

class CGUIListBox;

class CGUIScrollBar : public ox::gui::IGUIScrollBar
{
public:
    //! Sets the list box whose items the scroll bar scrolls.
    void setListBoxParent(CGUIListBox* parent);
};

} // end namespace gui
} // end namespace daisy

#endif
