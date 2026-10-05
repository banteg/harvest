// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the scenario interface used by world initialization and what CPlayState uses of the
// story screen, whose tail keeps the Linux object size.

#ifndef HARVEST_GUI_CSTORYSCREEN_H
#define HARVEST_GUI_CSTORYSCREEN_H

#include "ox/core/CString.h"
#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
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
    virtual ~CDialogueItemInfo() {}
    ox::core::CString<wchar_t> Name;
    ox::core::CString<wchar_t> Text;
    ox::core::CString<char> Portrait;
    ox::core::CString<char> Sound;
    float Time;
    bool Alternate;
};

//! Shows campaign dialogue over the game.
class CStoryScreen : public ox::event::IEventReceiver
{
public:
    CStoryScreen(ox::IOxDevice* device);
    virtual ~CStoryScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);
    bool isVisible();
    void setVisible(bool visible);
    void update(float frameDelta);
    void displayDialogueText(CDialogueItemInfo* item);
    void displayBlackness(float time);

private:
    // Not recovered yet; keeps the Linux object size of 192 bytes.
    char Unrecovered[192 - sizeof(ox::event::IEventReceiver)];
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
