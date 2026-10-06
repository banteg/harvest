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
//!
//! An input script can fix the step instead (setFixedStep): each read then advances the clock by
//! the step, so the game plays the same at any frame rate, a slow software renderer included.
class CSDLTimer : public daisy::CTimer
{
public:
    CSDLTimer() : FixedStep(0), FixedTime(0) {}

    virtual double getFloatTime()
    {
        if (FixedStep > 0)
            return FixedTime += FixedStep;
        return SDL_GetTicksNS() / 1000000000.0;
    }

    //! Seconds per frame from the next read on (counted from now), or 0 for the monotonic clock.
    void setFixedStep(double step)
    {
        FixedTime = SDL_GetTicksNS() / 1000000000.0;
        FixedStep = step;
    }

private:
    double FixedStep;
    double FixedTime;
};

} // end namespace port

#endif
