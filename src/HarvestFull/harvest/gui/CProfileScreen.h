// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Method names are from the Mac symbols; member names are ours.

#ifndef HARVEST_GUI_CPROFILESCREEN_H
#define HARVEST_GUI_CPROFILESCREEN_H

#include "ox/core/CString.h"
#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUIElement;
class IGUIEnvironment;
} // end namespace gui
namespace video { class IVideoDriver; }
} // end namespace ox

namespace harvest {
namespace gui {

//! The profile list and the window that creates or edits a profile.
class CProfileScreen : public ox::event::IEventReceiver
{
public:
    CProfileScreen(ox::IOxDevice* device);
    virtual ~CProfileScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    //! Opens the window that creates a profile, or with newProfile unset edits the current one.
    void createEditProfileWindow(bool newProfile);
    void removeWindow();
    //! Creates or changes the profile from the window's fields. With overwrite set, a profile of
    //! the same name is replaced instead of asked about.
    bool saveProfile(bool overwrite);
    void createProfileListWindow();
    void deleteCurrentProfile();
    bool isVisible();
    //! Replaces semicolons and trims spaces.
    void stripIllegalCharacters(ox::core::CString<wchar_t>& text);
    //! Shows a message box and returns false when the name is too short.
    bool checkValidName(ox::core::CString<wchar_t>& name);
    //! The text of the window element with the id.
    const wchar_t* getText(int id);

private:
    enum
    {
        ID_CREATE_PROFILE = 1545,
        ID_EDIT_PROFILE,
        ID_DELETE_PROFILE = 1548,
        ID_BACK,
        ID_PROFILE_LIST,
        ID_NAME,
        ID_GROUP,
        ID_SAVE,
        ID_CANCEL,
        ID_GROUP_INFO,
        ID_NAME_TAKEN,
        ID_CONFIRM_DELETE
    };

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    ox::gui::IGUIElement* Window;
    //! Set while the edit window changes the current profile instead of creating one.
    bool EditMode;
    //! The list box selection that was last opened.
    int SelectedIndex;
};

} // end namespace gui
} // end namespace harvest

#endif
