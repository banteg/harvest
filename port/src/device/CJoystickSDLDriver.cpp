#include "device/CJoystickSDLDriver.h"

namespace port {

namespace {

const int BUTTON_COUNT = 11;

//! The button numbers of SDL's first eleven gamepad buttons (the xpad order); later buttons keep
//! SDL's numbers, which continue from 11.
const int BUTTON_NUMBERS[BUTTON_COUNT] =
{
    0,  // SDL_GAMEPAD_BUTTON_SOUTH (A)
    1,  // SDL_GAMEPAD_BUTTON_EAST (B)
    2,  // SDL_GAMEPAD_BUTTON_WEST (X)
    3,  // SDL_GAMEPAD_BUTTON_NORTH (Y)
    6,  // SDL_GAMEPAD_BUTTON_BACK
    8,  // SDL_GAMEPAD_BUTTON_GUIDE
    7,  // SDL_GAMEPAD_BUTTON_START
    9,  // SDL_GAMEPAD_BUTTON_LEFT_STICK
    10, // SDL_GAMEPAD_BUTTON_RIGHT_STICK
    4,  // SDL_GAMEPAD_BUTTON_LEFT_SHOULDER
    5   // SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
};

int toButtonNumber(int button)
{
    return button < BUTTON_COUNT ? BUTTON_NUMBERS[button] : button;
}

SDL_GamepadButton toGamepadButton(int number)
{
    if (number < BUTTON_COUNT)
        for (int i = 0; i < BUTTON_COUNT; ++i)
            if (BUTTON_NUMBERS[i] == number)
                return (SDL_GamepadButton)i;
    return (SDL_GamepadButton)number;
}

const float STICK_DEAD_ZONE = 10.0f;

float stickPosition(Sint16 value)
{
    float position = SDL_clamp(value * (100.0f / 32767.0f), -100.0f, 100.0f);
    return SDL_fabsf(position) < STICK_DEAD_ZONE ? 0.0f : position;
}

} // end namespace

CJoystickSDLDriver::CJoystickSDLDriver()
{
    SDL_zeroa(Slots);
    int count = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&count);
    for (int i = 0; i < count; ++i)
        open(ids[i]);
    SDL_free(ids);
}

CJoystickSDLDriver::~CJoystickSDLDriver()
{
    for (int i = 0; i < MAX_JOYSTICKS; ++i)
        close(i);
}

int CJoystickSDLDriver::findSlot(SDL_JoystickID id)
{
    for (int i = 0; i < MAX_JOYSTICKS; ++i)
        if (Slots[i].Joystick && Slots[i].Id == id)
            return i;
    return -1;
}

int CJoystickSDLDriver::open(SDL_JoystickID id)
{
    int slot = findSlot(id);
    if (slot >= 0)
        return slot;

    for (slot = 0; slot < MAX_JOYSTICKS && Slots[slot].Joystick; ++slot)
    {
    }
    if (slot == MAX_JOYSTICKS)
        return -1;

    SSlot& s = Slots[slot];
    s.Id = id;
    if (SDL_IsGamepad(id))
    {
        s.Gamepad = SDL_OpenGamepad(id);
        s.Joystick = s.Gamepad ? SDL_GetGamepadJoystick(s.Gamepad) : 0;
    }
    else
        s.Joystick = SDL_OpenJoystick(id);

    if (!s.Joystick)
    {
        SDL_Log("cannot open joystick %u: %s", (unsigned int)id, SDL_GetError());
        return -1;
    }
    return slot;
}

void CJoystickSDLDriver::close(int slot)
{
    SSlot& s = Slots[slot];
    if (s.Gamepad)
        SDL_CloseGamepad(s.Gamepad);
    else if (s.Joystick)
        SDL_CloseJoystick(s.Joystick);
    s.Gamepad = 0;
    s.Joystick = 0;
}

int CJoystickSDLDriver::getNumAttachedJoysticks()
{
    int count = 0;
    for (int i = 0; i < MAX_JOYSTICKS; ++i)
        count += Slots[i].Joystick != 0;
    return count;
}

bool CJoystickSDLDriver::isButtonPressed(int joystick, int button)
{
    if (joystick < 0 || joystick >= MAX_JOYSTICKS || !Slots[joystick].Joystick || button < 0)
        return false;

    const SSlot& s = Slots[joystick];
    if (s.Gamepad)
        return SDL_GetGamepadButton(s.Gamepad, toGamepadButton(button));
    return SDL_GetJoystickButton(s.Joystick, button);
}

float CJoystickSDLDriver::getAxis(int slot, int axis)
{
    const SSlot& s = Slots[slot];
    if (!s.Gamepad)
        return axis < SDL_GetNumJoystickAxes(s.Joystick) ? stickPosition(SDL_GetJoystickAxis(s.Joystick, axis)) : 0.0f;

    switch (axis)
    {
    case 0:
        return stickPosition(SDL_GetGamepadAxis(s.Gamepad, SDL_GAMEPAD_AXIS_LEFTX));
    case 1:
        return stickPosition(SDL_GetGamepadAxis(s.Gamepad, SDL_GAMEPAD_AXIS_LEFTY));
    case 2:
        return SDL_GetGamepadAxis(s.Gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) * (200.0f / 32767.0f) - 100.0f;
    case 3:
        return SDL_GetGamepadAxis(s.Gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) * (200.0f / 32767.0f) - 100.0f;
    case 4:
        return stickPosition(SDL_GetGamepadAxis(s.Gamepad, SDL_GAMEPAD_AXIS_RIGHTX));
    case 5:
        return stickPosition(SDL_GetGamepadAxis(s.Gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
    case 6:
        return 100.0f * (SDL_GetGamepadButton(s.Gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
            - SDL_GetGamepadButton(s.Gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT));
    case 7:
        return 100.0f * (SDL_GetGamepadButton(s.Gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)
            - SDL_GetGamepadButton(s.Gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP));
    default:
        return 0.0f;
    }
}

ox::core::CVector3d<float> CJoystickSDLDriver::getJoystickAxes(int joystick, int stick)
{
    if (joystick < 0 || joystick >= MAX_JOYSTICKS || !Slots[joystick].Joystick || stick < 0)
        return ox::core::CVector3d<float>(0, 0, 0);

    int axis = stick / 3;
    return ox::core::CVector3d<float>(getAxis(joystick, axis), getAxis(joystick, axis + 1),
        getAxis(joystick, axis + 2));
}

bool CJoystickSDLDriver::translateEvent(const SDL_Event& event, ox::event::SEvent& out)
{
    out.EventType = ox::event::EET_JOYSTICK_INPUT_EVENT;
    out.JoystickEvent.Button = 0;

    switch (event.type)
    {
    case SDL_EVENT_JOYSTICK_ADDED:
        out.JoystickEvent.Type = 3;
        out.JoystickEvent.Joystick = open(event.jdevice.which);
        return out.JoystickEvent.Joystick >= 0;

    case SDL_EVENT_GAMEPAD_ADDED:
    {
        // A mapping can arrive after the joystick was opened raw.
        int slot = findSlot(event.gdevice.which);
        if (slot >= 0 && !Slots[slot].Gamepad)
        {
            SSlot& s = Slots[slot];
            s.Gamepad = SDL_OpenGamepad(s.Id);
            if (s.Gamepad)
            {
                SDL_CloseJoystick(s.Joystick);
                s.Joystick = SDL_GetGamepadJoystick(s.Gamepad);
            }
        }
        return false;
    }

    case SDL_EVENT_JOYSTICK_REMOVED:
        out.JoystickEvent.Type = 4;
        out.JoystickEvent.Joystick = findSlot(event.jdevice.which);
        if (out.JoystickEvent.Joystick < 0)
            return false;
        close(out.JoystickEvent.Joystick);
        return true;

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        out.JoystickEvent.Type = event.type == SDL_EVENT_GAMEPAD_BUTTON_UP;
        out.JoystickEvent.Joystick = findSlot(event.gbutton.which);
        out.JoystickEvent.Button = toButtonNumber(event.gbutton.button);
        return out.JoystickEvent.Joystick >= 0;

    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
    case SDL_EVENT_JOYSTICK_BUTTON_UP:
        // Gamepads report their buttons through the gamepad events above.
        out.JoystickEvent.Type = event.type == SDL_EVENT_JOYSTICK_BUTTON_UP;
        out.JoystickEvent.Joystick = findSlot(event.jbutton.which);
        out.JoystickEvent.Button = event.jbutton.button;
        return out.JoystickEvent.Joystick >= 0 && !Slots[out.JoystickEvent.Joystick].Gamepad;

    default:
        return false;
    }
}

} // end namespace port
