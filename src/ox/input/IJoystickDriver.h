// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac vtable of daisy::input::CJoystickMacDriver.

#ifndef OX_INPUT_IJOYSTICKDRIVER_H
#define OX_INPUT_IJOYSTICKDRIVER_H

#include "../IUnknown.h"
#include "../core/CVector3d.h"

namespace ox {
namespace input {

//! Gamepad input.
class IJoystickDriver : public IUnknown
{
public:
    virtual ~IJoystickDriver() {}

    virtual int getNumAttachedJoysticks() = 0;
    virtual bool isButtonPressed(int joystick, int button) = 0;
    //! The positions of three consecutive axes. The Linux driver returns SFML's raw positions,
    //! -100 to 100, of the axes stick / 3, stick / 3 + 1 and stick / 3 + 2 (see CJoystickLinuxDriver).
    virtual core::CVector3d<float> getJoystickAxes(int joystick, int stick) = 0;
    virtual void setXboxControllerVibration(int joystick, int left, int right) = 0;
};

} // end namespace input
} // end namespace ox

#endif
