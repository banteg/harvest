#include "device/Options.h"

#include <SDL3/SDL.h>
#include <string.h>

namespace port {

SOptions g_options;

static void printUsage(const char* program)
{
    SDL_Log("usage: %s [--data <dir>] [--null-video] [--no-audio] [--no-vsync] [--scale <factor>]\n"
            "       [--input-script <file>]\n"
            "  --data <dir>   the directory holding harvestClientData/ (default: $HARVEST_DATA, else the\n"
            "                 executable's directory)\n"
            "  --null-video   use the null video driver (nothing is drawn)\n"
            "  --no-audio     play no sound\n"
            "  --no-vsync     present frames as fast as possible\n"
            "  --scale <factor>\n"
            "                 screen pixels per game pixel, at least 1 (default: the display's scale,\n"
            "                 2 on Retina displays; 1 runs the game at the native resolution)\n"
            "  --input-script <file>\n"
            "                 synthetic input events for testing, one \"<frame> <command>\" per line",
        program);
}

bool parseOptions(int argc, char** argv)
{
    g_options.DataDirectory = SDL_getenv("HARVEST_DATA");

    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (!strcmp(arg, "--data") && i + 1 < argc)
            g_options.DataDirectory = argv[++i];
        else if (!strcmp(arg, "--null-video"))
            g_options.NullVideo = true;
        else if (!strcmp(arg, "--no-audio"))
            g_options.NoAudio = true;
        else if (!strcmp(arg, "--no-vsync"))
            g_options.VSync = false;
        else if (!strcmp(arg, "--scale") && i + 1 < argc && SDL_atof(argv[i + 1]) >= 1.0)
            g_options.Scale = (float)SDL_atof(argv[++i]);
        else if (!strcmp(arg, "--input-script") && i + 1 < argc)
            g_options.InputScript = argv[++i];
        else
        {
            printUsage(argv[0]);
            return false;
        }
    }

    return true;
}

} // end namespace port
