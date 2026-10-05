// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CGameMain.h"
#include "CGameState.h"
#include "ox/IOSOperator.h"
#include "ox/IOxDevice.h"
#include "ox/ITimer.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CString.h"
#include "ox/core/CThread.h"
#include "ox/event/IEventReceiver.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IReadFile.h"
#include "ox/video/IVideoDriver.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace game {

//! The state id that stops the game instead of creating a state.
static const int STATE_QUIT = 1;

CGameMain::CGameMain()
    : Running(false), State(0), SleepWhenInactive(true), LastTime(0)
{
    if (!event::gp_subscriberList)
        event::gp_subscriberList = new event::CEventSubscriberList();
}

CGameMain::~CGameMain()
{
    delete event::gp_subscriberList;
    event::gp_subscriberList = 0;
    clear();
}

void CGameMain::clear()
{
    if (State)
    {
        delete State;
        State = 0;
    }
}

void CGameMain::shutdown()
{
    Running = false;
}

void CGameMain::setState(int state)
{
    if (State)
    {
        delete State;
        State = 0;
    }

    if (state == STATE_QUIT)
    {
        Running = false;
        return;
    }

    State = stateFactory(state);
    if (State)
    {
        if (State->firstInit(Device) == 0)
        {
            while (true)
            {
                State->renderFirst();
                int result = State->secondInit();
                if (result == 1)
                    break;
                if (result != 2)
                {
                    State->subscribe(event::gp_subscriberList);
                    Device->setEventReceiver(event::gp_subscriberList);
                    return;
                }
            }
        }
        State->getErrorMessage();
    }
    Running = false;
}

CGameState* CGameMain::getState()
{
    return State;
}

bool CGameMain::isRunning()
{
    return Running;
}

float CGameMain::getNewTimeStep()
{
    double time = Device->getTimer()->getFloatTime();
    if (LastTime == 0 || LastTime > time)
        LastTime = time;
    double lastTime = LastTime;
    LastTime = time;
    return time - lastTime;
}

void CGameMain::updateMain(float time)
{
}

void CGameMain::update()
{
    if (!State || !Device || !Device->run())
    {
        Running = false;
        return;
    }

    float time = getNewTimeStep();

    if (Device->getAudioDriver())
        Device->getAudioDriver()->periodicStreamUpdate();

    int newState = 0;
    if (time > 0)
    {
        newState = State->updateState(time);
        updateMain(time);
        if (Device->isWindowActive() || !Device->getVideoDriver()->isFullscreen())
            State->render();
    }

    if (!Device->isWindowActive() && SleepWhenInactive)
        core::CThread::sleep(20);

    Device->pollNetworkDevices();

    if (event::gp_subscriberList)
        event::gp_subscriberList->executeDelayedEvents();

    if (newState)
        setState(newState);
}

bool CGameMain::checkFiles(char** files)
{
    if (!files || !Device)
        return true;

    for (int i = 0; files[i]; ++i)
    {
        io::IReadFile* file = Device->getFileSystem()->createAndOpenFile(files[i]);
        if (!file)
        {
            core::CString<wchar_t> message = L"File is missing: ";
            message.append(core::CString<wchar_t>(files[i]));
            message.append(core::CString<wchar_t>(L", please re-install."));
            Device->getOSOperator()->messageBox(L"Error", message.c_str(), 0);
            return false;
        }
        file->drop();
    }

    return true;
}

} // end namespace game
} // end namespace ox
