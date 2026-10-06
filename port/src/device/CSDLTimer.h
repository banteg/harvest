// The device clock of the SDL3 device.

#ifndef PORT_DEVICE_CSDLTIMER_H
#define PORT_DEVICE_CSDLTIMER_H

#include <SDL3/SDL.h>
#include "daisy/other/CTimer.h"

namespace port {

//! getTime stays the original's wall clock in milliseconds (os::Timer, gettimeofday): the GUI reads
//! os::Timer directly and compares with it, and the game seeds its random generator from it.
//! getFloatTime, which only CGameMain's time step reads, is SDL's monotonic clock in seconds, so a
//! wall-clock adjustment can no longer produce a zero or huge step.
class CSDLTimer : public daisy::CTimer
{
public:
    virtual double getFloatTime()
    {
        return SDL_GetTicksNS() / 1000000000.0;
    }
};

} // end namespace port

#endif
