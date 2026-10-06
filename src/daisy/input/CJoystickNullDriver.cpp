// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CJoystickNullDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace input {

CJoystickNullDriver::~CJoystickNullDriver()
{
}

int CJoystickNullDriver::getNumAttachedJoysticks()
{
    return 0;
}

bool CJoystickNullDriver::isButtonPressed(int joystick, int button)
{
    return false;
}

ox::core::CVector3d<float> CJoystickNullDriver::getJoystickAxes(int joystick, int stick)
{
    return ox::core::CVector3d<float>(0, 0, 0);
}

} // end namespace input
} // end namespace daisy
