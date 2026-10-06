// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_INPUT_CJOYSTICKNULLDRIVER_H
#define DAISY_INPUT_CJOYSTICKNULLDRIVER_H

#include "ox/input/IJoystickDriver.h"

namespace daisy {
namespace input {

//! A joystick driver without joysticks; the base of the platform drivers.
class CJoystickNullDriver : public ox::input::IJoystickDriver
{
public:
    virtual ~CJoystickNullDriver();

    virtual int getNumAttachedJoysticks();
    virtual bool isButtonPressed(int joystick, int button);
    virtual ox::core::CVector3d<float> getJoystickAxes(int joystick, int stick);
    //! Rumble is not supported by any of the 1.18 drivers.
    virtual void setXboxControllerVibration(int joystick, int left, int right) {}
};

} // end namespace input
} // end namespace daisy

#endif
