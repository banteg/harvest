// The Linux build's table (CIrrDeviceLinux's constructor) for the keys SFML knew, plus Windows
// virtual-key codes for the keys it left uninitialised. See docs/port/input-and-window.md.

#include "device/KeyMap.h"

namespace port {

namespace {

// Windows virtual-key codes the recovered EKEY_CODE enum does not name.
enum
{
    VK_MENU = 0x12,
    VK_PAUSE = 0x13,
    VK_SNAPSHOT = 0x2C,
    VK_LWIN = 0x5B,
    VK_RWIN = 0x5C,
    VK_APPS = 0x5D,
    VK_DECIMAL = 0x6E,
    VK_NUMLOCK = 0x90,
    VK_SCROLL = 0x91,
    VK_OEM_1 = 0xBA,      // ;
    VK_OEM_PLUS = 0xBB,   // =
    VK_OEM_2 = 0xBF,      // /
    VK_OEM_3 = 0xC0,      // `
    VK_OEM_4 = 0xDB,      // [
    VK_OEM_5 = 0xDC,      // backslash
    VK_OEM_6 = 0xDD,      // ]
    VK_OEM_7 = 0xDE       // '
};

} // end namespace

ox::EKEY_CODE toOxKey(SDL_Keycode key)
{
    // Letters and digits: SDL's codes are the lower-case characters.
    if (key >= SDLK_A && key <= SDLK_Z)
        return (ox::EKEY_CODE)(ox::KEY_KEY_A + (key - SDLK_A));
    if (key >= SDLK_0 && key <= SDLK_9)
        return (ox::EKEY_CODE)(ox::KEY_KEY_0 + (key - SDLK_0));
    if (key >= SDLK_KP_1 && key <= SDLK_KP_9)
        return (ox::EKEY_CODE)(ox::KEY_NUMPAD1 + (key - SDLK_KP_1));
    if (key >= SDLK_F1 && key <= SDLK_F12)
        return (ox::EKEY_CODE)(ox::KEY_F1 + (key - SDLK_F1));
    if (key >= SDLK_F13 && key <= SDLK_F24)
        return (ox::EKEY_CODE)(ox::KEY_F13 + (key - SDLK_F13));

    switch (key)
    {
    // The keys the Linux build mapped.
    case SDLK_KP_0: return ox::KEY_NUMPAD0;
    case SDLK_ESCAPE: return ox::KEY_ESCAPE;
    case SDLK_SPACE: return ox::KEY_SPACE;
    case SDLK_RETURN: return ox::KEY_RETURN;
    case SDLK_BACKSPACE: return ox::KEY_BACK;
    case SDLK_TAB: return ox::KEY_TAB;
    case SDLK_COMMA: return ox::KEY_COMMA;
    case SDLK_PERIOD: return ox::KEY_PERIOD;
    case SDLK_MINUS: return ox::KEY_MINUS;
    case SDLK_KP_PLUS: return ox::KEY_ADD;
    case SDLK_KP_MINUS: return ox::KEY_SUBTRACT;
    case SDLK_KP_MULTIPLY: return ox::KEY_MULTIPLY;
    case SDLK_KP_DIVIDE: return ox::KEY_DIVIDE;
    case SDLK_LEFT: return ox::KEY_LEFT;
    case SDLK_UP: return ox::KEY_UP;
    case SDLK_RIGHT: return ox::KEY_RIGHT;
    case SDLK_DOWN: return ox::KEY_DOWN;
    case SDLK_PAGEUP: return ox::KEY_PRIOR;
    case SDLK_PAGEDOWN: return ox::KEY_NEXT;
    case SDLK_END: return ox::KEY_END;
    case SDLK_HOME: return ox::KEY_HOME;
    case SDLK_DELETE: return ox::KEY_DELETE;

    // Linux sent KEY_CAPITAL for left shift (and garbage for right shift), so CPlayState's shift
    // checks (KEY_SHIFT, KEY_LSHIFT, KEY_RSHIFT) never fired there. The port sends the side-specific
    // codes and gives KEY_CAPITAL to Caps Lock, as Windows does.
    case SDLK_LSHIFT: return ox::KEY_LSHIFT;
    case SDLK_RSHIFT: return ox::KEY_RSHIFT;
    case SDLK_CAPSLOCK: return ox::KEY_CAPITAL;
    // CPlayState's control-click checks KEY_CONTROL, so both control keys send it (as Windows'
    // WM_KEYDOWN does); likewise both Alt keys send VK_MENU.
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        return ox::KEY_CONTROL;
    case SDLK_LALT:
    case SDLK_RALT:
        return (ox::EKEY_CODE)VK_MENU;

    // Keys the Linux build left uninitialised, with their Windows codes.
    case SDLK_INSERT: return ox::KEY_INSERT;
    case SDLK_PAUSE: return (ox::EKEY_CODE)VK_PAUSE;
    case SDLK_PRINTSCREEN: return (ox::EKEY_CODE)VK_SNAPSHOT;
    case SDLK_LGUI: return (ox::EKEY_CODE)VK_LWIN;
    case SDLK_RGUI: return (ox::EKEY_CODE)VK_RWIN;
    case SDLK_APPLICATION: return (ox::EKEY_CODE)VK_APPS;
    case SDLK_NUMLOCKCLEAR: return (ox::EKEY_CODE)VK_NUMLOCK;
    case SDLK_SCROLLLOCK: return (ox::EKEY_CODE)VK_SCROLL;
    case SDLK_SEMICOLON: return (ox::EKEY_CODE)VK_OEM_1;
    case SDLK_EQUALS: return (ox::EKEY_CODE)VK_OEM_PLUS;
    case SDLK_SLASH: return (ox::EKEY_CODE)VK_OEM_2;
    case SDLK_GRAVE: return (ox::EKEY_CODE)VK_OEM_3;
    case SDLK_LEFTBRACKET: return (ox::EKEY_CODE)VK_OEM_4;
    case SDLK_BACKSLASH: return (ox::EKEY_CODE)VK_OEM_5;
    case SDLK_RIGHTBRACKET: return (ox::EKEY_CODE)VK_OEM_6;
    case SDLK_APOSTROPHE: return (ox::EKEY_CODE)VK_OEM_7;
    // Keys SFML did not have: the keypad's Enter is Return and its point is VK_DECIMAL, as on Windows.
    case SDLK_KP_ENTER: return ox::KEY_RETURN;
    case SDLK_KP_PERIOD: return (ox::EKEY_CODE)VK_DECIMAL;

    default:
        return (ox::EKEY_CODE)0;
    }
}

} // end namespace port
