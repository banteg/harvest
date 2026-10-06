// Gamepads and joysticks through SDL3 (CJoystickLinuxDriver's replacement).

#ifndef PORT_DEVICE_CJOYSTICKSDLDRIVER_H
#define PORT_DEVICE_CJOYSTICKSDLDRIVER_H

#include <SDL3/SDL.h>
#include "daisy/input/CJoystickNullDriver.h"
#include "ox/event/IEventReceiver.h"

namespace port {

//! Up to eight joysticks in numbered slots (SFML's sf::Joystick::Count), filled in connection order.
//!
//! Devices SDL knows as gamepads report the button numbers and axes the Linux build saw from an
//! Xbox pad through SFML (the xpad driver's order): buttons A 0, B 1, X 2, Y 3, LB 4, RB 5, Back 6,
//! Start 7, Guide 8, left stick 9, right stick 10, then SDL's order (D-pad up, down, left, right
//! 11-14, ...); axes 0 left X, 1 left Y, 2 left trigger, 3 right trigger, 4 right X, 5 right Y,
//! 6 D-pad X, 7 D-pad Y (SFML's X, Y, Z, R, U, V, PovX, PovY). Other joysticks report SDL's raw
//! button and axis numbers. Positions are SFML's -100 to 100 (triggers -100 released, 100 pressed);
//! sticks get a dead zone of 10, since the game treats any deflection past 0.5 as a turn.
class CJoystickSDLDriver : public daisy::input::CJoystickNullDriver
{
public:
    CJoystickSDLDriver();
    virtual ~CJoystickSDLDriver();

    virtual int getNumAttachedJoysticks();
    virtual bool isButtonPressed(int joystick, int button);
    //! Axes stick / 3, +1 and +2, as the Linux driver read them (the game only passes 0).
    virtual ox::core::CVector3d<float> getJoystickAxes(int joystick, int stick);

    //! Turns SDL's joystick and gamepad events into the ox joystick event: type 0 for a button press,
    //! 1 for a release, 3 for a connection, 4 for a disconnection. Returns false for events that post
    //! nothing.
    bool translateEvent(const SDL_Event& event, ox::event::SEvent& out);

private:
    enum { MAX_JOYSTICKS = 8 };

    struct SSlot
    {
        SDL_JoystickID Id;
        SDL_Joystick* Joystick;
        //! Set when SDL has a gamepad mapping for the device.
        SDL_Gamepad* Gamepad;
    };

    int findSlot(SDL_JoystickID id);
    //! Opens the device in the first free slot; returns the slot or -1.
    int open(SDL_JoystickID id);
    void close(int slot);
    float getAxis(int slot, int axis);

    SSlot Slots[MAX_JOYSTICKS];
};

} // end namespace port

#endif
