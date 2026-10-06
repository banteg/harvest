// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// PROVISIONAL: only the JPEG writer CVideoOpenGL::saveJpegScreenshot calls is declared; the full
// header of the CImageLoaderJPG unit replaces this one.

#ifndef DAISY_VIDEO_NULL_CIMAGELOADERJPG_H
#define DAISY_VIDEO_NULL_CIMAGELOADERJPG_H

namespace ox {
namespace io { class IWriteFile; }
} // end namespace ox

namespace daisy {
namespace video {

class CImageLoaderJPG
{
public:
    //! Writes width x height pixels of the given number of 8 bit channels (3: RGB) as a JPEG.
    static bool saveImage(ox::io::IWriteFile* file, unsigned char* data, int width, int height, int channels);
};

} // end namespace video
} // end namespace daisy

#endif
