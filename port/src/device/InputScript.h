// --input-script <file>: synthetic SDL events for testing the port without a person at the
// keyboard. See docs/port/input-and-window.md.

#ifndef PORT_DEVICE_INPUTSCRIPT_H
#define PORT_DEVICE_INPUTSCRIPT_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>

namespace port {

//! Lines of "<frame> <command> [arguments]", run when that many frames have passed with no state
//! loading (# starts a comment). Positions are window coordinates (points on macOS):
//! - move <x> <y>: the pointer moves there (a motion event; the system pointer stays put)
//! - click <x> <y>: left button pressed and released there
//! - wheel <x> <y> [flipped]: SDL's precise wheel amounts at the pointer, as a trackpad sends them
//! - text <words>: a text input event with the words joined by single spaces
//! - key <name>: a key pressed and released; SDL key names, "ctrl+" for Control (Command on macOS)
//! - size <w> <h>: SDL_SetWindowSize
//! - display <n>: the window moves to the middle of the n-th display
//! - fullscreen on|off: SDL_SetWindowFullscreen
//! - clock <rate>|real: the game clock advances 1/rate seconds per frame from now on, so the script
//!   plays the same at any frame rate; "real" returns to the monotonic clock
//! - screenshot <name>: the next frame drawn (after the events of the same frame's commands) as
//!   $HARVEST_USERDATA$/screenshots/<name>-<yymmdd>-NN.jpg (saveJpegScreenshot, as Ctrl+T calls it)
//! - quit: a quit request
class CInputScript
{
public:
    CInputScript() : Next(0), Frame(0), PointerX(0), PointerY(0) {}

    //! Reads the file; false (logged) when it cannot be read or a line cannot be parsed.
    bool load(const char* path);
    //! Pushes the events of the commands due at this frame, then counts it.
    void runFrame(SDL_Window* window);
    bool isFinished() const { return Next >= Commands.size(); }

private:
    struct SCommand
    {
        unsigned int Frame;
        std::string Name;
        std::vector<std::string> Arguments;
        //! The arguments joined by single spaces (text's string).
        std::string Text;
    };

    void run(const SCommand& command, SDL_Window* window);

    std::vector<SCommand> Commands;
    unsigned int Next;
    unsigned int Frame;
    float PointerX;
    float PointerY;
};

} // end namespace port

#endif
