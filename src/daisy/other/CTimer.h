// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTimer.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_CTIMER_H
#define DAISY_CTIMER_H

#include "daisy/include/ITimer.h"
#include "daisy/os.h"

namespace daisy {

//! The device timer; both clocks read the wall clock through os::Timer (see os.cpp).
class CTimer : public ITimer
{
public:
    CTimer()
    {
        os::Timer::initTimer();
    }

    //! Milliseconds of the wall clock, truncated to 32 bits.
    virtual unsigned int getTime()
    {
        return os::Timer::getTime();
    }

    //! Seconds of the wall clock with microsecond resolution.
    virtual double getFloatTime()
    {
        return os::Timer::getFloatTime();
    }
};

} // end namespace daisy

#endif
