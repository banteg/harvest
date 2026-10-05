// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CGUIINFOLINES_H
#define HARVEST_GUI_CGUIINFOLINES_H

#include <list>

namespace ox {
class IOxDevice;
namespace gui {
class IGUIElement;
class IGUIEnvironment;
class IGUIFont;
class IGUILayout;
} // end namespace gui
namespace video {
class ISpritePackage;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace gui {

//! One message of the info line list.
struct SInfoLine
{
    //! Seconds until the line is removed.
    float TimeLeft;
    ox::gui::IGUIElement* Element;
    //! Display time of a plain text line, from its length.
    float Duration;
};

//! The in-game message list that stacks the info lines up from the bottom of the screen.
class CGuiInfoLines
{
public:
    CGuiInfoLines(ox::IOxDevice* device);
    virtual ~CGuiInfoLines();

    //! Ages the lines and removes the expired ones; the list ends at the given bottom.
    void update(float time, int bottom);
    void addInfoLine(const wchar_t* text);
    //! Adds a framed line with an icon, a bold title and a text.
    void addInfoLine(const wchar_t* title, const wchar_t* text, ox::video::ISpritePackage* sprites,
        const char* icon);

private:
    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    ox::gui::IGUIFont* SmallFont;
    int SmallFontHeight;
    ox::gui::IGUIFont* BoldFont;
    int BoldFontHeight;
    ox::gui::IGUILayout* Container;
    std::list<SInfoLine*> InfoLines;
    //! Keeps the lines that scrolled near the top until the first update.
    bool FirstUpdate;
};

} // end namespace gui
} // end namespace harvest

#endif
