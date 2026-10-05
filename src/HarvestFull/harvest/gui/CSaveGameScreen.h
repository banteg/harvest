// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CSAVEGAMESCREEN_H
#define HARVEST_GUI_CSAVEGAMESCREEN_H

#include <vector>
#include "harvest/settings/CSavestateInfo.h"
#include "ox/core/CString.h"
#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUIButton;
class IGUIEditBox;
class IGUIElement;
class IGUIEnvironment;
class IGUILayout;
class IGUIListBox;
} // end namespace gui
namespace video {
class ISpriteAnimationState;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace gui {

//! A save game of the current profile, as listed by the save screen.
struct SSaveListItem
{
    ox::core::CString<char> Filename;
    //! The file name without the directory.
    ox::core::CString<wchar_t> Name;
    settings::SSavestateHeader Header;

    // The written-out destructor and assignment reproduce GCC's inlining order in the sort helpers.
    ~SSaveListItem()
    {
    }

    SSaveListItem& operator=(const SSaveListItem& other)
    {
        Filename = other.Filename;
        Name = other.Name;
        Header = other.Header;
        return *this;
    }
};

//! Sorts the newest save game first.
struct SSaveGameSorter
{
    bool operator()(const SSaveListItem& a, const SSaveListItem& b) const
    {
        return a.Header.Time > b.Header.Time;
    }
};

//! The screen that lists the save games of the current profile to load one or save over one.
class CSaveGameScreen : public ox::event::IEventReceiver
{
public:
    CSaveGameScreen(ox::IOxDevice* device);
    virtual ~CSaveGameScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    //! Shows or hides the screen; loading selects between the load and the save screen.
    void setVisible(bool visible, bool loading);
    bool isVisible();
    //! The file and description chosen to save the game to.
    const char* getSelectedSaveFilename();
    const wchar_t* getSelectedSaveDescription();

private:
    enum
    {
        ID_BACK = 68881,
        //! Loads or saves to the selected list item.
        ID_ACTION,
        ID_DELETE,
        ID_SAVE,
        ID_CANCEL,
        ID_CONFIRM_DELETE,
        ID_CONFIRM_OVERWRITE,
        //! The planet icons of the list items, by planet.
        ID_PLANET_ICON,
        //! The game mode icons of the list items, by game mode.
        ID_MODE_ICON = ID_PLANET_ICON + 3
    };

    enum
    {
        MODE_ICON_COUNT = 5,
        PLANET_ICON_COUNT = 3
    };

    //! Loads or saves to the selected list item, asking before overwriting a save game.
    void performListAction();
    //! Fills the list with the save games of the current profile.
    void loadSaveGames();
    //! Asks for the description of a new save game.
    void createSaveGameBox();
    void addSavestateSlot(const settings::SSavestateHeader& header, const ox::core::CString<wchar_t>& name);
    void addEmptySlot();

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    ox::gui::IGUIElement* Window;
    ox::gui::IGUILayout* Frame;
    ox::gui::IGUIButton* ActionButton;
    ox::gui::IGUIListBox* ListBox;
    std::vector<SSaveListItem> SaveList;
    bool Loading;
    //! The save game the delete question is about.
    ox::core::CString<char> DeleteFilename;
    ox::core::CString<char> SelectedFilename;
    ox::core::CString<wchar_t> SelectedDescription;
    //! The modal screen asking for the description of a save game.
    ox::gui::IGUIElement* SaveBox;
    ox::gui::IGUIEditBox* DescriptionBox;
    ox::video::ISpriteAnimationState* ModeIcons[MODE_ICON_COUNT];
    ox::video::ISpriteAnimationState* PlanetIcons[PLANET_ICON_COUNT];
};

} // end namespace gui
} // end namespace harvest

#endif
