// The factories that join the port's platform pieces. The device (SDL3) owns the window, the GL
// context and the main loop, and creates the other pieces through these functions; each piece is
// implemented in its own directory under port/src/.

#ifndef PORT_PLATFORM_SEAMS_H
#define PORT_PLATFORM_SEAMS_H

#include "ox/core/CDimension2d.h"

namespace ox {
class IOxDevice;
namespace audio { class IAudioDriver; }
namespace io { class IFileSystem; }
namespace video { class IVideoDriver; }
} // end namespace ox

namespace port {

//! Creates the renderer (port/src/video/). The device calls it with its window's OpenGL ES 3.0 /
//! OpenGL 3.3 core context current. The renderer derives from daisy::video::CVideoNull, and its
//! endScene presents the frame through device->swapBuffers(), as the original OpenGL driver did.
ox::video::IVideoDriver* createVideoDriver(ox::IOxDevice* device, ox::io::IFileSystem* fileSystem,
    const ox::core::CDimension2d<int>& screenSize);

//! Creates the audio backend (port/src/audio/), a daisy::audio::CAudioDriver. The device calls it
//! from CIrrDeviceStub::createAudioDriver on first use.
ox::audio::IAudioDriver* createAudioDriver(ox::io::IFileSystem* fileSystem);

} // end namespace port

// The scene manager the main menu uses (port/src/scene/) keeps the original's factory,
// daisy::scene::createSceneManager, declared in src/daisy/other/CIrrDeviceStub.cpp.

#endif
