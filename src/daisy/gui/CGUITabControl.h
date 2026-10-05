// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUITabControl.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUITABCONTROL_H
#define DAISY_GUI_CGUITABCONTROL_H

#include <vector>
#include "ox/gui/IGUITabControl.h"

namespace ox {
namespace video { class ISpriteAnimationState; }
}

namespace daisy {
namespace gui {

// A tab, onto which other gui elements could be added.
class CGUITab : public ox::gui::IGUITab
{
public:
    //! constructor
    CGUITab(int number, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
        const ox::core::CRect<int>& rectangle, int id);

    //! destructor
    virtual ~CGUITab();

    //! Returns number of this tab in tabcontrol. Can be accessed
    //! later IGUITabControl::getTab() by this number.
    virtual int getNumber();

    //! Sets the number
    virtual void setNumber(int n);

    //! draws the element and its children
    virtual void draw();

    //! sets if the tab should draw its background
    virtual void setDrawBackground(bool draw = true);

    //! sets the color of the background, if it should be drawn.
    virtual void setBackgroundColor(ox::video::SColor c);

private:
    int Number;
    bool DrawBackground;
    ox::video::SColor BackColor;
};

//! The pieces of a tab control's sprite skin, in the order of setAnimations' names.
enum ETAB_CONTROL_ANIMATION
{
    ETCA_BACKGROUND = 0,
    ETCA_TOP_LEFT,
    ETCA_TOP_RIGHT,
    ETCA_BOTTOM_LEFT,
    ETCA_BOTTOM_RIGHT,
    ETCA_TOP,
    ETCA_LEFT,
    ETCA_RIGHT,
    ETCA_BOTTOM,
    ETCA_TAB,
    ETCA_COUNT
};

//! A standard tab control, drawn with a sprite skin when it has one.
class CGUITabControl : public ox::gui::IGUITabControl
{
public:
    //! constructor
    CGUITabControl(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
        const ox::core::CRect<int>& rectangle, bool fillbackground = true, bool border = true, int id = -1);

    //! destructor
    virtual ~CGUITabControl();

    //! Adds a tab
    virtual ox::gui::IGUITab* addTab(wchar_t* caption, int id = -1);

    //! Returns amount of tabs in the tabcontrol
    virtual int getTabcount();

    //! Returns a tab based on zero based index
    virtual ox::gui::IGUITab* getTab(int idx);

    //! Brings a tab to front.
    virtual bool setActiveTab(int idx);

    //! Returns which tab is currently active
    virtual int getActiveTab();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! Removes a child.
    virtual void removeChild(ox::gui::IGUIElement* child);

    //! Skins the control with the package's animations named name followed by the piece names.
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);

    virtual void setTextColor(ox::video::SColor color);

private:
    void selectTab(ox::core::CPosition2d<int> p);

    std::vector<CGUITab*> Tabs;
    int ActiveTab;
    bool Border;
    bool FillBackground;
    ox::video::ISpriteAnimationState* Animations[ETCA_COUNT];
    //! Where each animation is drawn, relative to the control.
    ox::core::CRect<int> Rects[ETCA_COUNT];
    ox::video::SColor TextColor;
};

} // end namespace gui
} // end namespace daisy

#endif
