// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IImageLoader.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of ox::video::IImageLoader.

#ifndef OX_VIDEO_IIMAGELOADER_H
#define OX_VIDEO_IIMAGELOADER_H

#include "../IUnknown.h"

namespace ox {
namespace io {
class IReadFile;
} // end namespace io

namespace video {

class IImage;

//! Loads one image file format into a software image.
class IImageLoader : public IUnknown
{
public:
    //! Whether the file name has an extension this loader handles.
    virtual bool isALoadableFileExtension(const char* fileName) = 0;
    //! Whether the file's contents look like this loader's format.
    virtual bool isALoadableFileFormat(io::IReadFile* file) = 0;
    //! Loads the image; the caller owns the returned reference.
    virtual IImage* loadImage(io::IReadFile* file) = 0;
};

} // end namespace video
} // end namespace ox

#endif
