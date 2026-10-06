#include "device/InputScript.h"

#include <stdlib.h>
#include <string.h>

namespace port {

namespace {

void push(SDL_Event& event, SDL_Window* window, Uint32 type)
{
    event.type = type;
    event.common.timestamp = SDL_GetTicksNS();
    event.window.windowID = SDL_GetWindowID(window);
    SDL_PushEvent(&event);
}

float toFloat(const std::string& text)
{
    return (float)SDL_atof(text.c_str());
}

} // end namespace

bool CInputScript::load(const char* path)
{
    size_t size = 0;
    char* text = (char*)SDL_LoadFile(path, &size);
    if (!text)
    {
        SDL_Log("cannot read the input script %s: %s", path, SDL_GetError());
        return false;
    }

    int lineNumber = 0;
    bool valid = true;

    // one command per line: "<frame> <command> [arguments]"; # starts a comment
    char* save = 0;
    for (char* line = SDL_strtok_r(text, "\n", &save); line; line = SDL_strtok_r(0, "\n", &save))
    {
        ++lineNumber;
        char* comment = strchr(line, '#');
        if (comment)
            *comment = 0;

        SCommand command;
        char* wordSave = 0;
        char* frame = SDL_strtok_r(line, " \t\r", &wordSave);
        if (!frame)
            continue;
        char* name = SDL_strtok_r(0, " \t\r", &wordSave);
        if (!name)
        {
            SDL_Log("%s:%d: expected <frame> <command>", path, lineNumber);
            valid = false;
            continue;
        }
        command.Frame = (unsigned int)SDL_atoi(frame);
        command.Name = name;
        for (char* word = SDL_strtok_r(0, " \t\r", &wordSave); word; word = SDL_strtok_r(0, " \t\r", &wordSave))
            command.Arguments.push_back(word);
        Commands.push_back(command);
    }

    SDL_free(text);
    return valid;
}

void CInputScript::runFrame(SDL_Window* window)
{
    while (Next < Commands.size() && Commands[Next].Frame <= Frame)
        run(Commands[Next++], window);
    ++Frame;
}

void CInputScript::run(const SCommand& command, SDL_Window* window)
{
    const std::vector<std::string>& args = command.Arguments;
    SDL_Event event;
    SDL_zero(event);

    if (command.Name == "move" && args.size() == 2)
    {
        PointerX = toFloat(args[0]);
        PointerY = toFloat(args[1]);
        event.motion.x = PointerX;
        event.motion.y = PointerY;
        push(event, window, SDL_EVENT_MOUSE_MOTION);
    }
    else if (command.Name == "click" && args.size() == 2)
    {
        PointerX = toFloat(args[0]);
        PointerY = toFloat(args[1]);
        event.button.button = SDL_BUTTON_LEFT;
        event.button.clicks = 1;
        event.button.x = PointerX;
        event.button.y = PointerY;
        event.button.down = true;
        push(event, window, SDL_EVENT_MOUSE_BUTTON_DOWN);
        event.button.down = false;
        push(event, window, SDL_EVENT_MOUSE_BUTTON_UP);
    }
    else if (command.Name == "wheel" && (args.size() == 2 || args.size() == 3))
    {
        event.wheel.x = toFloat(args[0]);
        event.wheel.y = toFloat(args[1]);
        event.wheel.direction = args.size() == 3 && args[2] == "flipped" ? SDL_MOUSEWHEEL_FLIPPED
                                                                          : SDL_MOUSEWHEEL_NORMAL;
        event.wheel.mouse_x = PointerX;
        event.wheel.mouse_y = PointerY;
        push(event, window, SDL_EVENT_MOUSE_WHEEL);
    }
    else if (command.Name == "key" && args.size() == 1)
    {
        std::string key = args[0];
        if (key.compare(0, 5, "ctrl+") == 0)
        {
            event.key.mod = SDL_KMOD_LCTRL;
            key = key.substr(5);
        }
        event.key.key = SDL_GetKeyFromName(key.c_str());
        event.key.scancode = SDL_GetScancodeFromKey(event.key.key, 0);
        event.key.down = true;
        push(event, window, SDL_EVENT_KEY_DOWN);
        event.key.down = false;
        push(event, window, SDL_EVENT_KEY_UP);
    }
    else if (command.Name == "size" && args.size() == 2)
        SDL_SetWindowSize(window, SDL_atoi(args[0].c_str()), SDL_atoi(args[1].c_str()));
    else if (command.Name == "display" && args.size() == 1)
    {
        int count = 0;
        SDL_DisplayID* displays = SDL_GetDisplays(&count);
        int index = SDL_atoi(args[0].c_str());
        if (displays && index >= 0 && index < count)
        {
            SDL_Log("input script: moving the window to display %d (%s, scale %g)", index,
                SDL_GetDisplayName(displays[index]), SDL_GetDisplayContentScale(displays[index]) *
                    SDL_GetDesktopDisplayMode(displays[index])->pixel_density);
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(displays[index]),
                SDL_WINDOWPOS_CENTERED_DISPLAY(displays[index]));
        }
        else
            SDL_Log("input script: there is no display %d", index);
        SDL_free(displays);
    }
    else if (command.Name == "fullscreen" && args.size() == 1)
        SDL_SetWindowFullscreen(window, args[0] == "on");
    else if (command.Name == "quit")
        push(event, window, SDL_EVENT_QUIT);
    else
        SDL_Log("input script: cannot run frame %u's %s", command.Frame, command.Name.c_str());
}

} // end namespace port
