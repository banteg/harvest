// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the 1.18 builds never create a thread pool, so only the virtual destructor that
// CIrrDeviceStub's destructor calls is known.

#ifndef OX_ITHREADPOOL_H
#define OX_ITHREADPOOL_H

namespace ox {

//! A pool of worker threads, owned and deleted by the device.
class IThreadPool
{
public:
    virtual ~IThreadPool() {}
};

} // end namespace ox

#endif
