// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CSETTINGSSCREEN_H
#define HARVEST_GUI_CSETTINGSSCREEN_H

#include "ox/event/IEventReceiver.h"
#include "ox/core/CDimension2d.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUIElement;
class IGUILayout;
} // end namespace gui
} // end namespace ox

namespace harvest {
namespace gui {

//! The settings window: audio, video and language settings on one tab, key bindings on another.
class CSettingsScreen : public ox::event::IEventReceiver
{
public:
    //! With languageSelection set, the general tab also lists the language files.
    CSettingsScreen(ox::IOxDevice* device, bool languageSelection);
    virtual ~CSettingsScreen();

    //! Shows or hides the window; showing it loads the current settings into the controls.
    void setVisible(bool visible);
    //! Loads the current settings into the controls.
    void setValues();
    bool isVisible();

    virtual bool OnEvent(const ox::event::SEvent& event);

private:
    enum
    {
        ID_DONE = 1534,
        ID_SFX_VOLUME,
        ID_MUSIC_VOLUME,
        ID_PARTICLES,
        ID_SCROLL_SPEED,
        ID_RESOLUTION,
        ID_LANGUAGE,
        ID_FULLSCREEN,
        ID_RESET_KEYS,
        //! The key binding buttons are ID_KEY_COMMAND plus the settings::EKeyCommands value.
        ID_KEY_COMMAND = 89832789
    };

    int getSliderValue(int id);
    bool getCheckboxChecked(int id);
    int getListSelection(int id);
    void setSliderValue(int id, int value);
    void setCheckboxChecked(int id, bool checked);
    //! Selects the resolution in the list box.
    void setListSelection(int id, const ox::core::CDimension2d<int>& resolution);
    //! Shows the bound key on each key binding button.
    void updateKeyMappings();
    void loadDefaultValues();
    //! Applies the selected resolution and fullscreen mode to the device window.
    void applyScreenMode();
    //! Opens the modal "press a key" box shown while a key binding is changed.
    void createLoadBlock();

    ox::IOxDevice* Device;
    ox::gui::IGUILayout* Window;
    ox::gui::IGUIElement* LoadBlock;
    //! The resolutions listed in the resolution list box, in list order.
    ox::core::CDimension2d<int>* Resolutions;
    int ResolutionCount;
    //! The command whose key binding is being changed, or -1.
    int KeyCommand;
    //! The language list selection when the window was opened.
    int LanguageSelection;
};

} // end namespace gui
} // end namespace harvest

#endif
