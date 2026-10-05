// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The implementation lives in CGUITabControl.cpp, as in the Linux build.

#ifndef DAISY_GUI_CGUITABBUTTONROW_H
#define DAISY_GUI_CGUITABBUTTONROW_H

#include <vector>
#include "ox/gui/IGUITabButtonRow.h"

namespace ox {
namespace video { class ISpriteAnimationState; }
}

namespace daisy {
namespace gui {

//! A tab of a CGUITabButtonRow.
struct STabRowTabInfo
{
    ox::core::CString<wchar_t> Caption;
    //! The active tab shows a close button.
    bool Closable;
    //! The caption is drawn in the highlight color.
    bool Highlighted;
    //! The left edge, relative to the row.
    int X;
};

//! The pieces of a tab button row's sprite skin, in the order of setAnimations' names.
enum ETAB_BUTTON_ROW_ANIMATION
{
    ETBRA_TOP = 0,
    ETBRA_TAB_NORMAL,
    ETBRA_TAB_HIGHLIGHTED,
    ETBRA_TOP_INNER,
    ETBRA_CLOSE_NORMAL,
    ETBRA_CLOSE_HIGHLIGHTED,
    ETBRA_COUNT
};

//! A row of overlapping tab buttons; the active tab is drawn on top and can be closed.
class CGUITabButtonRow : public ox::gui::IGUITabButtonRow
{
public:
    CGUITabButtonRow(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
        const ox::core::CRect<int>& rectangle, int id);

    virtual ~CGUITabButtonRow();

    virtual void setRelativePosition(const ox::core::CRect<int>& position);
    virtual void draw();
    virtual bool OnEvent(const ox::event::SEvent& event);

    virtual int addTab(const wchar_t* caption, bool closable);
    virtual int getTabCount();
    virtual const wchar_t* getTabCaption(int index);
    virtual void setActiveTabButton(int index);
    virtual void setTabButtonHighlight(int index, bool highlight);
    virtual int getActiveTabButton();
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);
    virtual void setTextColor(ox::video::SColor color);
    virtual void setTextFont(ox::gui::IGUIFont* font);
    virtual void setHighlightColor(ox::video::SColor color);

private:
    //! Spreads the tabs over the row, overlapping them when they do not fit.
    void repositionTabs();

    std::vector<STabRowTabInfo*> Tabs;
    int ActiveTab;
    //! The tab under the mouse, or -1.
    int HoverTab;
    //! The mouse is over the close button of the hovered tab.
    bool HoverClose;
    ox::video::ISpriteAnimationState* Animations[ETBRA_COUNT];
    ox::core::CRect<int> Rects[ETBRA_COUNT];
    //! Where the close button is drawn in the active tab.
    ox::core::CPosition2d<int> ClosePosition;
    ox::video::SColor TextColor;
    ox::video::SColor HighlightColor;
    ox::gui::IGUIFont* TextFont;
};

} // end namespace gui
} // end namespace daisy

#endif
