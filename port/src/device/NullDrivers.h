// The drivers behind --null-video and --no-audio.

#ifndef PORT_DEVICE_NULLDRIVERS_H
#define PORT_DEVICE_NULLDRIVERS_H

#include "ox/core/CDimension2d.h"

namespace ox {
class IOxDevice;
namespace audio { class IAudioDriver; }
namespace io { class IFileSystem; }
namespace video { class IVideoDriver; }
} // end namespace ox

namespace port {

//! daisy's null driver (it loads textures and sprite packages but draws nothing) that clears the
//! window to the scene colour, presents through the device and remembers the fullscreen flag.
ox::video::IVideoDriver* createNullVideoDriver(ox::IOxDevice* device, ox::io::IFileSystem* fileSystem,
    const ox::core::CDimension2d<int>& screenSize);

//! A daisy::audio::CAudioDriver whose backend loads nothing, so every play call does nothing.
ox::audio::IAudioDriver* createNullAudioDriver();

} // end namespace port

#endif
