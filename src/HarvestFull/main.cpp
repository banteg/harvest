// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
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
