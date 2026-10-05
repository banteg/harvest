// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CSTORYSCREEN_H
#define HARVEST_GUI_CSTORYSCREEN_H

#include "ox/algo/CTimeCounter.h"
#include "ox/core/CString.h"
#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUIElement;
class IGUIEnvironment;
class IGUILayout;
class IGUIStaticText;
} // end namespace gui
namespace video { class IVideoDriver; }
} // end namespace ox

namespace harvest {
namespace gui {

//! Campaign dialogue and credits, including display duration and alternating credit alignment.
class CDialogueItemInfo
{
public:
    CDialogueItemInfo(const wchar_t* name, const wchar_t* text, const char* portrait,
                      const char* sound, float time, bool alternate)
        : Name(name), Text(text), Portrait(portrait), Sound(sound), Time(time), Alternate(alternate) {}
    CDialogueItemInfo(CDialogueItemInfo* other)
        : Name(other->Name), Text(other->Text), Portrait(other->Portrait), Sound(other->Sound),
          Time(other->Time), Alternate(other->Alternate) {}
    virtual ~CDialogueItemInfo() {}
    ox::core::CString<wchar_t> Name;
    ox::core::CString<wchar_t> Text;
    ox::core::CString<char> Portrait;
    ox::core::CString<char> Sound;
    float Time;
    bool Alternate;
};

//! The letterbox borders that show campaign dialogue, one line at the top and one at the bottom,
//! and the fade to black between scenes.
class CStoryScreen : public ox::event::IEventReceiver
{
public:
    CStoryScreen(ox::IOxDevice* device);
    virtual ~CStoryScreen();

    void update(float time);
    bool isVisible();
    void setVisible(bool visible);
    //! Fades the screen in from black over a delay.
    void displayBlackness(float delay);
    //! Shows a copy of the dialogue at the top, or at the bottom for an alternate speaker.
    void displayDialogueText(CDialogueItemInfo* info);

    virtual bool OnEvent(const ox::event::SEvent& event);

private:
    enum
    {
        ID_WINDOW = 4558,
        ID_TOP_BORDER,
        ID_BOTTOM_BORDER
    };

    //! The border states.
    enum
    {
        STATE_HIDDEN,
        STATE_OPENING,
        STATE_OPEN,
        STATE_CLOSING
    };

    void activateBorderText(int index);
    void createBorderText(int index, ox::gui::IGUIElement* parent);

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    ox::gui::IGUIElement* Window;
    ox::algo::CTimeCounter Blackness;
    int State;
    //! How far the borders are open, from 0 to 1.
    float Fade;

public:
    //! Closes the borders once both dialogues have run out.
    bool CloseWhenDone;

private:
    ox::gui::IGUIElement* TopBorder;
    ox::gui::IGUIElement* BottomBorder;
    ox::gui::IGUILayout* Border[2];
    ox::gui::IGUILayout* TextGroup[2];
    ox::gui::IGUIStaticText* Portrait[2];
    ox::gui::IGUIStaticText* NameText[2];
    ox::gui::IGUIStaticText* DialogueText[2];
    CDialogueItemInfo* Dialogues[2];
};

} // end namespace gui
namespace game {

class CWorld;

class IScenario
{
public:
    virtual ~IScenario() {}
    virtual int getDoodadSeed() const = 0;
    virtual void applyInitialExpansions(CWorld* world) = 0;
};

} // end namespace game
} // end namespace harvest

#endif
