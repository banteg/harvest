// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUITABCONTROL_H
#define DAISY_GUI_CGUITABCONTROL_H

#include "ox/gui/IGUITabControl.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUITabControl : public ox::gui::IGUITabControl
{
public:
    CGUITabControl(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, const ox::core::CRect<int>& rectangle, bool fillBackground, bool border, int id);
    virtual ~CGUITabControl();
    virtual ox::gui::IGUITab* addTab(const wchar_t* caption, int id);
    virtual int getTabcount();
    virtual ox::gui::IGUITab* getTab(int index);
    virtual bool setActiveTab(int index);
    virtual int getActiveTab();
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);
    virtual void setTextColor(ox::video::SColor color);

private:
    char Unrecovered[0x1d0 - sizeof(ox::gui::IGUITabControl)];
};

//! A page of a tab control.
class CGUITab : public ox::gui::IGUITab
{
public:
    CGUITab(int number, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, const ox::core::CRect<int>& rectangle, int id);
    virtual ~CGUITab();

private:
    char Unrecovered[0xc8 - sizeof(ox::gui::IGUITab)];
};


} // end namespace gui
} // end namespace daisy

#endif
