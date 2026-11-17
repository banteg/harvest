// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// PROVISIONAL: the JPEG writer CVideoOpenGL::saveJpegScreenshot calls and the libjpeg callbacks
// are declared; the full header of the CImageLoaderJPG unit replaces this one.

#ifndef DAISY_VIDEO_NULL_CIMAGELOADERJPG_H
#define DAISY_VIDEO_NULL_CIMAGELOADERJPG_H
#include "../../../ox/video/IImageLoader.h"

#include <stdio.h>
#define _IRR_COMPILE_WITH_LIBJPEG_
extern "C" {
#include <jconfig.h>
#include <jpeglib.h>
}

namespace ox {
namespace io { class IWriteFile; }
} // end namespace ox

namespace daisy {
namespace video {

class CImageLoaderJPG : public ox::video::IImageLoader
{
public:
    CImageLoaderJPG();
    virtual ~CImageLoaderJPG();
    virtual bool isALoadableFileExtension(const char* fileName);
    virtual bool isALoadableFileFormat(ox::io::IReadFile* file);
    virtual ox::video::IImage* loadImage(ox::io::IReadFile* file);
    //! Writes width x height pixels of the given number of 8 bit channels (3: RGB) as a JPEG.
    static bool saveImage(ox::io::IWriteFile* file, unsigned char* data, int width, int height, int channels);
private:
    static void init_source (j_decompress_ptr cinfo);
    static boolean fill_input_buffer (j_decompress_ptr cinfo);
    static void skip_input_data (j_decompress_ptr cinfo, long num_bytes);
    static void resync_to_restart (j_decompress_ptr cinfo, long desired);
    static void term_source (j_decompress_ptr cinfo);
};

} // end namespace video
} // end namespace daisy

#endif
