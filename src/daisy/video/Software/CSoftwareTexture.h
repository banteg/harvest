// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSoftwareTexture.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SOFTWARE_TEXTURE_H_INCLUDED__
#define __C_SOFTWARE_TEXTURE_H_INCLUDED__

#include "ox/video/ITexture.h"
#include "ox/core/CDimension2d.h"
#include "ox/video/IImage.h"
#include "daisy/video/Null/CImage.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video  
{
using namespace ox::video;

/*!
	interface for a Video Driver dependent Texture.
*/
class CSoftwareTexture : public ITexture
{
public:

	//! constructor
	CSoftwareTexture(IImage* surface);

	//! destructor
	virtual ~CSoftwareTexture();

	//! lock function
	virtual void* lock();

	//! unlock function
	virtual void unlock();

	//! Returns original size of the texture.
	virtual const ox::core::CDimension2d<int>& getOriginalSize();

	//! Returns (=size) of the texture.
	virtual const ox::core::CDimension2d<int>& getSize();

	//! returns unoptimized surface
	virtual CImage* getImage();

	//! returns texture surface
	virtual CImage* getTexture();

	//! returns driver type of texture (=the driver, who created the texture)
	virtual int getDriverType();

	//! returns color format of texture
	virtual int getColorFormat();

	//! returns pitch of texture (in bytes)
	virtual int getPitch();

private:

	//! returns the size of a texture which would be the optimize size for rendering it
	inline int getTextureSizeFromSurfaceSize(int size);

	CImage* Image;
	CImage* Texture;
	ox::core::CDimension2d<int> OrigSize;

};


} // end namespace video
} // end namespace daisy

#endif

