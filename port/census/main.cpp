// Stub entry point for the link census: the same loop as src/HarvestFull/main.cpp, which the port
// replaces with SDL3's main callbacks.

#include "harvest/CHarvestFullMain.h"

int main()
{
    harvest::CHarvestFullMain game;
    if (game.init() == 0)
        while (game.isRunning())
            game.update();
    game.clear();
    return 0;
}
