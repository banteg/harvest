// Recovered for Harvest from the Linux 1.18 build; not the original source.

#ifndef DAISY_INPUT_CJOYSTICKLINUXDRIVER_H
#define DAISY_INPUT_CJOYSTICKLINUXDRIVER_H

#include "CJoystickNullDriver.h"

namespace daisy {
namespace input {

//! Joysticks through SFML's sf::Joystick (up to 8 joysticks, SFML's axis and button numbering).
class CJoystickLinuxDriver : public CJoystickNullDriver
{
public:
    CJoystickLinuxDriver();
    virtual ~CJoystickLinuxDriver();

    virtual int getNumAttachedJoysticks();
    virtual bool isButtonPressed(int joystick, int button);
    virtual ox::core::CVector3d<float> getJoystickAxes(int joystick, int stick);

    //! Refreshes SFML's joystick states; the device calls it at the start of every run().
    void update();
};

} // end namespace input
} // end namespace daisy

#endif
