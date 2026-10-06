// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_ITIMER_H
#define DAISY_ITIMER_H

#include "ox/ITimer.h"
#include "ox/IUnknown.h"

namespace daisy {

//! The device clock: ox::ITimer plus reference counting (the IUnknown base sits after the ITimer
//! vtable pointer).
class ITimer : public ox::ITimer, public ox::IUnknown
{
public:
    virtual ~ITimer() {}
};

} // end namespace daisy

#endif
