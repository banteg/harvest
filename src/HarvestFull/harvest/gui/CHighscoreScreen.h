// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CHIGHSCORESCREEN_H
#define HARVEST_GUI_CHIGHSCORESCREEN_H

#include "ox/event/IEventReceiver.h"
#include "ox/core/CCriticalSection.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CString.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUICheckBox;
class IGUIEditBox;
class IGUIElement;
class IGUIEnvironment;
class IGUILayout;
class IGUIListBox;
class IGUIStaticText;
} // end namespace gui
namespace net { class CHTTPConnectionHandler; }
namespace video
{
class ISpriteAnimationState;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace gui {

//! One promoted player, peer or group of a summary page.
struct SSummaryEntry
{
    ox::core::CString<wchar_t> Name;
    ox::core::CString<wchar_t> Group;
    ox::core::CString<wchar_t> Score;
};

//! The best entries of a summary category, by game mode, score or minerals, and planet.
struct SSummaryPage
{
    SSummaryEntry Entries[4][2][4];
};

//! The online highscore screen: a summary page per category and the full highscore table.
class CHighscoreScreen : public ox::event::IEventReceiver
{
public:
    CHighscoreScreen(ox::IOxDevice* device);
    virtual ~CHighscoreScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    void setVisible(bool visible);
    bool isVisible();
    //! Parses a downloaded page and shows it.
    void update(float frameDelta);

private:
    enum
    {
        ID_BACK = 75609,
        ID_SCREEN,
        ID_TYPE_FRAME,
        //! The players, peers and groups summaries and the full table.
        ID_TYPE_BUTTON,
        ID_SUMMARY_FRAME = ID_TYPE_BUTTON + 4,
        ID_LIST_FRAME,
        //! The ten buttons of the table header and footer.
        ID_LIST_BUTTON,
        ID_POPUP_CANCEL = ID_LIST_BUTTON + 10,
        ID_POPUP_RANK,
        ID_POPUP_NAME,
        ID_POPUP_GROUP,
        ID_POPUP_PLANET,
        ID_POPUP_MODE,
        //! The promote buttons of a summary page, by mode, score or minerals, and planet.
        ID_PROMOTE
    };

    void parseSummaryPage(const ox::core::CString<char>& page);
    void parseHighscoreString(const ox::core::CString<char>& page);
    void createSummaryPage();
    ox::gui::IGUILayout* createNewPopupWindow();
    void sortAndMovePopup(ox::gui::IGUILayout* popup, ox::gui::IGUIElement* button, bool above);
    void loadHighscores(int offset);
    void loadSummaryPage(int category);
    void loadSprites();
    void createLoadBlock();
    ox::core::CString<char> createBase64ForUCS2(const ox::core::CString<wchar_t>& text);
    ox::core::CString<wchar_t> createUCS2FromBase64UTF2(const ox::core::CString<char>& text);
    ox::core::CString<wchar_t> parseRelativeTimeFormat(const ox::core::CString<char>& seconds);
    void createStatusString(const wchar_t* text);

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    //! The id of the highlighted type button.
    int SelectedType;
    //! Not waiting for a download.
    bool Ready;
    //! The download is a summary page.
    bool SummaryMode;
    int GameMode;
    int Planet;
    int Unknown38;
    int SortMode;
    ox::core::CString<wchar_t> NameFilter;
    ox::core::CString<wchar_t> GroupFilter;
    bool ExactName;
    bool ExactGroup;
    int Offset;
    SSummaryPage SummaryPages[3];
    bool SummaryLoaded[3];
    int SummaryCategory;
    int ShownSummaryCategory;
    bool SpritesLoaded;
    ox::video::ISpriteAnimationState* Sprites[24];
    ox::core::CPosition2d<int> SpriteSizes[24];
    ox::gui::IGUIElement* Window;
    ox::gui::IGUIElement* Background;
    ox::gui::IGUIElement* TypeFrame;
    ox::gui::IGUIElement* TypeButtons[4];
    ox::core::CString<wchar_t> TypeNames[4];
    ox::gui::IGUIElement* ListFrame;
    ox::gui::IGUIStaticText* StatusText;
    ox::gui::IGUIElement* SummaryFrame;
    ox::gui::IGUIElement* LoadBlock;
    ox::gui::IGUIElement* Cells[20][7];
    ox::gui::IGUIElement* PromoteButtons[4][2][4];
    ox::gui::IGUIElement* Popup;
    ox::gui::IGUIEditBox* RankEditBox;
    ox::gui::IGUIEditBox* FilterEditBox;
    ox::gui::IGUICheckBox* ExactCheckBox;
    ox::gui::IGUIListBox* PlanetList;
    ox::gui::IGUIListBox* ModeList;
    ox::net::CHTTPConnectionHandler* Connection;
    ox::core::CCriticalSection Lock;
    //! The downloaded page, written by the connection thread.
    ox::core::CString<char> Response;
};

} // end namespace gui
} // end namespace harvest

#endif
