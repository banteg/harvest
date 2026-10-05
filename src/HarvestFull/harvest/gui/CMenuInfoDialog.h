// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CMENUINFODIALOG_H
#define HARVEST_GUI_CMENUINFODIALOG_H

#include "ox/core/CString.h"
#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUIElement;
class IGUIEnvironment;
class IGUILayout;
} // end namespace gui
namespace video { class IVideoDriver; }
} // end namespace ox

namespace harvest {
namespace gui {

//! The briefing dialog of the first scenario, with the voiced welcome and debriefing calls.
class CMenuInfoDialog : public ox::event::IEventReceiver
{
public:
    enum EInfoMode
    {
        EIM_WELCOME,
        EIM_DEBRIEFING
    };

    CMenuInfoDialog(ox::IOxDevice* device);
    virtual ~CMenuInfoDialog();

    virtual bool OnEvent(const ox::event::SEvent& event);

    void setVisible(bool visible, int mode);
    void update(float time);
    bool isVisible();

private:
    enum
    {
        ID_START_TUTORIAL = 780,
        ID_CLOSE,
        ID_SYNDICATE_TEXT,
        ID_INFO_TEXT
    };

    void refillMainView(int mode);

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    //! The music started by the dialog.
    ox::core::CString<char> Music;
    ox::gui::IGUIElement* Window;
    ox::gui::IGUILayout* Frame;
    ox::gui::IGUIElement* MainView;
    int Mode;
    //! Time left until the first and second speaker finish their line.
    float SyndicateTime;
    float InfoTime;
};

} // end namespace gui
} // end namespace harvest

#endif
