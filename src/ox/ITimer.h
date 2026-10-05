// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only the slot used by recovered code is declared.

#ifndef OX_ITIMER_H
#define OX_ITIMER_H

namespace ox {

//! The device clock.
class ITimer
{
public:
    //! Milliseconds since the device started.
    virtual unsigned int getTime() = 0;
};

} // end namespace ox

#endif
