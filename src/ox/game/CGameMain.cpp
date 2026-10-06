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
#ifdef HARVEST_PORT
#include "ox/event/ILogger.h"
#endif
#include "ox/video/IVideoDriver.h"
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace game {

//! The state id that stops the game instead of creating a state.
static const int STATE_QUIT = 1;

CGameMain::CGameMain()
    : Running(false), State(0), SleepWhenInactive(true), LastTime(0)
#ifdef HARVEST_PORT
    , Loading(false)
#endif
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
#ifdef HARVEST_PORT
            // The original loops over the steps below inside this call. The port runs one step per
            // update (loadStep), so a frame never blocks for the whole load: the web build cannot
            // block, and the window keeps answering its events.
            Loading = true;
            return;
#endif
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
#ifdef HARVEST_PORT
        failState();
        return;
#endif
        State->getErrorMessage();
    }
    Running = false;
}

#ifdef HARVEST_PORT
bool CGameMain::isLoading()
{
    return Loading;
}

//! One iteration of the original's loading loop in setState: secondInit returns 2 for more steps,
//! 1 for a failure, anything else when the state is ready.
void CGameMain::loadStep()
{
    State->renderFirst();
    int result = State->secondInit();
    if (result == 2)
        return;

    Loading = false;
    if (result == 1)
    {
        failState();
        return;
    }
    State->subscribe(event::gp_subscriberList);
    Device->setEventReceiver(event::gp_subscriberList);
}

//! The original fetches the message and drops it; the port logs it.
void CGameMain::failState()
{
    Device->getLogger()->log("The game state failed to load", State->getErrorMessage(), event::ELL_ERROR);
    Running = false;
}
#endif

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

#ifdef HARVEST_PORT
    if (Loading)
    {
        loadStep();
        return;
    }
#endif

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

#if !defined(HARVEST_PORT) || !defined(__EMSCRIPTEN__)
    // The web build must not block; the browser throttles hidden pages itself.
    if (!Device->isWindowActive() && SleepWhenInactive)
        core::CThread::sleep(20);
#endif

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
