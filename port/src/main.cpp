// The port's entry point: src/HarvestFull/main.cpp's loop, run from SDL3's main callbacks so the
// frame step never blocks (the web build needs that; every platform uses it).
//
// SDL dispatches the queued events to SDL_AppEvent on the main thread right before each
// SDL_AppIterate, which runs one game frame (CGameMain::update) or, while a state loads, one loading
// step. The original's device pumped the same events at the start of update, so the order is the
// same. The original processed no events while a state loaded (the whole load ran inside one call);
// the port holds the input events back until loading ends and replays them then, in order.

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <vector>
#include "device/CIrrDeviceSDL.h"
#include "device/InputScript.h"
#include "device/Options.h"
#include "harvest/CHarvestFullMain.h"

namespace {

struct SApp
{
    harvest::CHarvestFullMain Game;
    //! Input that arrived while a state was loading.
    std::vector<SDL_Event> HeldEvents;
    //! --input-script's events.
    port::CInputScript InputScript;
};

bool isInput(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_TEXT_INPUT:
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
    case SDL_EVENT_JOYSTICK_BUTTON_UP:
        return true;
    default:
        return false;
    }
}

void handleEvent(const SDL_Event& event)
{
    port::CIrrDeviceSDL* device = port::CIrrDeviceSDL::getInstance();
    if (device)
        device->handleEvent(event);
}

} // end namespace

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    if (!port::parseOptions(argc, argv))
        return SDL_APP_FAILURE;

    SApp* app = new SApp();
    *appstate = app;
    if (port::g_options.InputScript && !app->InputScript.load(port::g_options.InputScript))
        return SDL_APP_FAILURE;
    // init creates the device (createDevice), the window and the first state, which then loads
    // over the following iterations.
    if (app->Game.init() != 0)
        return SDL_APP_FAILURE;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    SApp* app = static_cast<SApp*>(appstate);
    if (!app->Game.isRunning())
        return SDL_APP_SUCCESS;

    app->Game.update();

    if (!app->Game.isLoading() && !app->HeldEvents.empty())
    {
        std::vector<SDL_Event> held;
        held.swap(app->HeldEvents);
        for (unsigned int i = 0; i < held.size(); ++i)
            handleEvent(held[i]);
    }

    // the script's frames count while no state loads; its events arrive before the next frame
    port::CIrrDeviceSDL* device = port::CIrrDeviceSDL::getInstance();
    if (!app->Game.isLoading() && !app->InputScript.isFinished() && device && device->getWindow())
        app->InputScript.runFrame(device->getWindow());
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    SApp* app = static_cast<SApp*>(appstate);
    if (app->Game.isLoading() && isInput(*event))
        app->HeldEvents.push_back(*event);
    else
        handleEvent(*event);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult)
{
    SApp* app = static_cast<SApp*>(appstate);
    if (!app)
        return;
    app->Game.clear();
    delete app;
}
