// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSoftwareTexture.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/IOxDevice.h"
#include "ox/video/IImage.h"
#include "ox/event/ILogger.h"
#include "CSoftwareTexture.h"
#include "daisy/os.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;

//! constructor
CSoftwareTexture::CSoftwareTexture(IImage* image)
: Texture(0)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! destructor
CSoftwareTexture::~CSoftwareTexture()
{
	if (Image)
		Image->drop();

	if (Texture)
		Texture->drop();
}



//! lock function
void* CSoftwareTexture::lock()
{
	return Image->lock();
}



//! unlock function
void CSoftwareTexture::unlock()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! Returns original size of the texture.
const ox::core::CDimension2d<int>& CSoftwareTexture::getOriginalSize()
{
	return OrigSize;
}


//! Returns (=size) of the texture.
const ox::core::CDimension2d<int>& CSoftwareTexture::getSize()
{
	return Image->getDimension();
}


//! returns unoptimized surface
CImage* CSoftwareTexture::getImage()
{
	return Image;
}



//! returns texture surface
CImage* CSoftwareTexture::getTexture()
{
	return Texture;
}



//! returns the size of a texture which would be the optimize size for rendering it
inline int CSoftwareTexture::getTextureSizeFromSurfaceSize(int size)
{
	int ts = 0x01;
	while(ts < size)
		ts <<= 1;

	return ts;
}



//! returns driver type of texture (=the driver, who created the texture)
int CSoftwareTexture::getDriverType()
{
	return EDT_SOFTWARE;
}



//! returns color format of texture
int CSoftwareTexture::getColorFormat()
{
	return ECF_A1R5G5B5;
}



//! returns pitch of texture (in bytes)
int CSoftwareTexture::getPitch()
{
	return Image->getDimension().Width * 2;
}



} // end namespace video
} // end namespace daisy
