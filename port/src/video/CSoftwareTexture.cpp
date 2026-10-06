// The software texture CVideoNull::createDeviceDependentTexture falls back to. The port's renderer
// overrides createDeviceDependentTexture, so this is never created; it is an empty texture.

#include "daisy/video/Software/CSoftwareTexture.h"

namespace daisy {
namespace video {

namespace {
const ox::core::CDimension2d<int> EmptySize(0, 0);
} // end anonymous namespace

CSoftwareTexture::CSoftwareTexture(ox::video::IImage*)
{
}

CSoftwareTexture::~CSoftwareTexture()
{
}

void* CSoftwareTexture::lock()
{
    return 0;
}

void CSoftwareTexture::unlock()
{
}

const ox::core::CDimension2d<int>& CSoftwareTexture::getOriginalSize()
{
    return EmptySize;
}

const ox::core::CDimension2d<int>& CSoftwareTexture::getSize()
{
    return EmptySize;
}

int CSoftwareTexture::getDriverType()
{
    return 0;
}

int CSoftwareTexture::getColorFormat()
{
    return 0;
}

int CSoftwareTexture::getPitch()
{
    return 0;
}

} // end namespace video
} // end namespace daisy
