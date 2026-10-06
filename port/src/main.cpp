// The port's entry point: src/HarvestFull/main.cpp's loop, run from SDL3's main callbacks so the
// frame step never blocks (the web build needs that; every platform uses it).
//
// Skeleton: the SDL3 device fills in event delivery (SDL_AppEvent) and anything else the device
// needs from here. The game state lives in one heap object owned by the callbacks.

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "harvest/CHarvestFullMain.h"

SDL_AppResult SDL_AppInit(void** appstate, int, char**)
{
    harvest::CHarvestFullMain* game = new harvest::CHarvestFullMain();
    *appstate = game;
    // init creates the device (ox::createDevice), the window and the first game state.
    if (game->init() != 0)
        return SDL_APP_FAILURE;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    harvest::CHarvestFullMain* game = static_cast<harvest::CHarvestFullMain*>(appstate);
    if (!game->isRunning())
        return SDL_APP_SUCCESS;
    game->update();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event)
{
    // TODO(device): hand events to the SDL3 device, which turns them into ox events.
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult)
{
    harvest::CHarvestFullMain* game = static_cast<harvest::CHarvestFullMain*>(appstate);
    if (!game)
        return;
    game->clear();
    delete game;
}
