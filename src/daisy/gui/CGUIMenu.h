// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIMenu.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIMENU_H
#define DAISY_GUI_CGUIMENU_H

#include "CGUIContextMenu.h"

namespace daisy {
namespace gui {

//! GUI menu interface.
class CGUIMenu : public CGUIContextMenu
{
public:
    //! constructor
    CGUIMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);

    //! destructor
    ~CGUIMenu();

    //! draws the element and its children
    virtual void draw();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! Updates the absolute position.
    virtual void updateAbsolutePosition();

protected:
    virtual void recalculateSize();

    //! returns the item highlight-area
    virtual ox::core::CRect<int> getHRect(SItem& i, ox::core::CRect<int>& absolute);

    //! Gets drawing rect of Item
    virtual ox::core::CRect<int> getRect(SItem& i, ox::core::CRect<int>& absolute);

    void closeAllSubMenus();
};

} // end namespace gui
} // end namespace daisy

#endif
