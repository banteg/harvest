// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_GUI_CGUICLICKAREA_H
#define DAISY_GUI_CGUICLICKAREA_H

#include "ox/gui/IGUIClickArea.h"

namespace daisy {
namespace gui {

//! An invisible element that turns mouse button events over it into GUI events for its parent.
class CClickArea : public ox::gui::IGUIClickArea
{
public:
    CClickArea(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);

    virtual ~CClickArea();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the children
    virtual void draw();
};

} // end namespace gui
} // end namespace daisy

#endif
