// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only what CGUIMessageBox uses (the constructor, the overrides it calls, the close button
// and the size) is recovered here. The CGUIWindow unit owns the full declaration.

#ifndef DAISY_GUI_CGUIWINDOW_H
#define DAISY_GUI_CGUIWINDOW_H

#include "ox/gui/IGUIWindow.h"
#include "ox/gui/IGUIButton.h"

namespace daisy {
namespace gui {

//! A window with a title bar and close, minimize and maximize buttons.
class CGUIWindow : public ox::gui::IGUIWindow
{
public:
    //! The meaning of the last flag is not recovered here.
    CGUIWindow(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool flag);
    ~CGUIWindow();

    virtual void setRelativePosition(const ox::core::CRect<int>& position);
    virtual void draw();
    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual ox::core::CRect<int> getContentArea();
    virtual ox::gui::IGUIButton* getCloseButton();
    virtual ox::gui::IGUIButton* getMinimizeButton();
    virtual ox::gui::IGUIButton* getMaximizeButton();
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* animation);

protected:
    char UnrecoveredHead[0xc8 - sizeof(ox::gui::IGUIWindow)];
    ox::gui::IGUIButton* CloseButton;
    // The Linux object is 0x218 bytes (CGUIMessageBox's members follow).
    char UnrecoveredTail[0x218 - 0xd0];
};

} // end namespace gui
} // end namespace daisy

#endif
