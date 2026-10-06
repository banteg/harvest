// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_IDAISYDEVICE_H
#define DAISY_IDAISYDEVICE_H

#include "ox/IOxDevice.h"

namespace daisy {

//! The engine device as daisy implements it; adds nothing to ox::IOxDevice.
class IDaisyDevice : public ox::IOxDevice
{
public:
    virtual ~IDaisyDevice() {}
};

} // end namespace daisy

#endif
