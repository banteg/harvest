// Recovered for Harvest from the Linux 1.18 build; not the original source.

#include "CJoystickLinuxDriver.h"
#include <SFML/Window.hpp>
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace input {

CJoystickLinuxDriver::CJoystickLinuxDriver()
{
}

CJoystickLinuxDriver::~CJoystickLinuxDriver()
{
}

//! Counts the connected joysticks among SFML's eight slots; a gap does not stop the count.
int CJoystickLinuxDriver::getNumAttachedJoysticks()
{
    int count = 0;
    for (int i = 0; i < sf::Joystick::Count; ++i)
        count += sf::Joystick::isConnected(i);
    return count;
}

//! SFML's button numbering.
bool CJoystickLinuxDriver::isButtonPressed(int joystick, int button)
{
    return sf::Joystick::isButtonPressed(joystick, button);
}

//! The raw SFML positions (-100 to 100, not -1 to 1) of three consecutive axes starting at axis
//! stick / 3: sticks 0 to 2 all read X, Y and Z. Callers compare X with +-0.5, so any deflection
//! past half a percent counts.
ox::core::CVector3d<float> CJoystickLinuxDriver::getJoystickAxes(int joystick, int stick)
{
    int axis = stick / 3;
    return ox::core::CVector3d<float>(sf::Joystick::getAxisPosition(joystick, (sf::Joystick::Axis)axis),
        sf::Joystick::getAxisPosition(joystick, (sf::Joystick::Axis)(axis + 1)),
        sf::Joystick::getAxisPosition(joystick, (sf::Joystick::Axis)(axis + 2)));
}

void CJoystickLinuxDriver::update()
{
    sf::Joystick::update();
}

} // end namespace input
} // end namespace daisy
