// The PSD and PCX loaders CVideoNull creates. The game ships no such files, so the port's loaders
// accept nothing. They must be objects: CVideoNull calls every loader and drops them. The BMP
// loader, which the GUI's built-in font needs, is in ImageLoaderBmp.cpp.

#include "ox/video/IImageLoader.h"

namespace daisy {
namespace video {

namespace {

class CNullImageLoader : public ox::video::IImageLoader
{
public:
    virtual bool isALoadableFileExtension(const char*)
    {
        return false;
    }

    virtual bool isALoadableFileFormat(ox::io::IReadFile*)
    {
        return false;
    }

    virtual ox::video::IImage* loadImage(ox::io::IReadFile*)
    {
        return 0;
    }
};

} // end anonymous namespace

ox::video::IImageLoader* createImageLoaderPSD()
{
    return new CNullImageLoader();
}

ox::video::IImageLoader* createImageLoaderPCX()
{
    return new CNullImageLoader();
}

} // end namespace video
} // end namespace daisy
