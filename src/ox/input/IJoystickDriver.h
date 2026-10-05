// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac vtable of daisy::input::CJoystickMacDriver.

#ifndef OX_INPUT_IJOYSTICKDRIVER_H
#define OX_INPUT_IJOYSTICKDRIVER_H

#include "../core/CVector2d.h"

namespace ox {
namespace input {

//! Gamepad input.
class IJoystickDriver
{
public:
    virtual ~IJoystickDriver() {}

    virtual int getNumAttachedJoysticks() = 0;
    virtual bool isButtonPressed(int joystick, int button) = 0;
    //! The position of a stick, each axis from -1 to 1.
    virtual core::CVector2d<float> getJoystickAxes(int joystick, int stick) = 0;
    virtual void setXboxControllerVibration(int joystick, int left, int right) = 0;
};

} // end namespace input
} // end namespace ox

#endif
