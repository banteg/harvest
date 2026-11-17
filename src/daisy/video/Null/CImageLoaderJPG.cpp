#include <iostream>
#include "CImageLoaderJPG.h"
#include "ox/core/CDimension2d.h"
#include "ox/video/ColorPacking.h"
#include "ox/video/IImage.h"
#include "ox/video/IImageLoader.h"
#include "ox/io/IReadFile.h"
#include "ox/io/IWriteFile.h"
#include <string.h>
#include "CColorConverter.h"
#include "CImage.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;

//! constructor
CImageLoaderJPG::CImageLoaderJPG()
{
}



//! destructor
CImageLoaderJPG::~CImageLoaderJPG()
{
}



//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderJPG::isALoadableFileExtension(const char* fileName)
{
	return strstr(fileName, ".jpg") != 0;
}


#ifdef _IRR_COMPILE_WITH_LIBJPEG_
void CImageLoaderJPG::init_source (j_decompress_ptr cinfo)
{
	// DO NOTHING
}



boolean CImageLoaderJPG::fill_input_buffer (j_decompress_ptr cinfo)
{
	// DO NOTHING
	return 1;
}



void CImageLoaderJPG::skip_input_data (j_decompress_ptr cinfo, long count)
{
	jpeg_source_mgr * src = cinfo->src;
	if(count > 0)
	{
		src->bytes_in_buffer -= count;
		src->next_input_byte += count;
	}
}



void CImageLoaderJPG::resync_to_restart (j_decompress_ptr cinfo, long desired)
{
	// DO NOTHING
}



void CImageLoaderJPG::term_source (j_decompress_ptr cinfo)
{
	// DO NOTHING
}
#endif


//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderJPG::isALoadableFileFormat(ox::io::IReadFile* file)
{
    #ifndef _IRR_COMPILE_WITH_LIBJPEG_

    return false;

    #else

	if (!file)
		return false;

	int jfif = 0;
	file->seek(6);
	file->read(&jfif, sizeof(int));
	return (jfif == 0x4a464946 || jfif == 0x4649464a);

	#endif
}



//! creates a surface from the file
IImage* CImageLoaderJPG::loadImage(ox::io::IReadFile* file)
{
    #ifndef _IRR_COMPILE_WITH_LIBJPEG_
    return 0;
    #else

	file->seek(0);
	unsigned char* input = new unsigned char[file->getSize()];
	file->read(input, file->getSize());

	struct jpeg_decompress_struct cinfo;

	// allocate and initialize JPEG decompression object
	struct jpeg_error_mgr jerr;

	//We have to set up the error handler first, in case the initialization
	//step fails.  (Unlikely, but it could happen if you are out of memory.)
	//This routine fills in the contents of struct jerr, and returns jerr's
	//address which we place into the link field in cinfo.

	cinfo.err = jpeg_std_error(&jerr);
	// Now we can initialize the JPEG decompression object.
	jpeg_create_decompress(&cinfo);

	// specify data source
	jpeg_source_mgr jsrc;

	// Set up data pointer
	jsrc.bytes_in_buffer = file->getSize();
	jsrc.next_input_byte = (JOCTET*)input;
	cinfo.src = &jsrc;

	jsrc.init_source = init_source;
	jsrc.fill_input_buffer = fill_input_buffer;
	jsrc.skip_input_data = skip_input_data;
	jsrc.resync_to_restart = jpeg_resync_to_restart;
	jsrc.term_source = term_source;

	// Decodes JPG input from whatever source
	// Does everything AFTER jpeg_create_decompress
	// and BEFORE jpeg_destroy_decompress
	// Caller is responsible for arranging these + setting up cinfo

	// read file parameters with jpeg_read_header()
	(void) jpeg_read_header(&cinfo, TRUE);

	// Start decompressor
	(void) jpeg_start_decompress(&cinfo);

	// Get image data
	unsigned short rowspan = cinfo.image_width * cinfo.num_components;
	unsigned width = cinfo.image_width;
	unsigned height = cinfo.image_height;

	bool has_alpha= false;  //(JPEG never has alpha)
	bool greyscale;

	if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
		greyscale = true;
	else
		greyscale = false;

	// Allocate memory for buffer
	unsigned char *output = new unsigned char[rowspan * height];

	// Here we use the library's state variable cinfo.output_scanline as the
	// loop counter, so that we don't have to keep track ourselves.
	// Create array of row pointers for lib
	unsigned char **rowPtr = new unsigned char * [height];

	for( unsigned i = 0; i < height; i++ )
		rowPtr[i] = &output[ i * rowspan ];

	unsigned rowsRead = 0;

	while( cinfo.output_scanline < cinfo.output_height )
		rowsRead += jpeg_read_scanlines( &cinfo, &rowPtr[rowsRead], cinfo.output_height - rowsRead );

	delete [] rowPtr;
	// Finish decompression

	(void) jpeg_finish_decompress(&cinfo);

	// Release JPEG decompression object

	// This is an important step since it will release a good deal of memory.
	jpeg_destroy_decompress(&cinfo);


	// convert image
	IImage* image = new CImage(ECF_R8G8B8,
		ox::core::CDimension2d<int>(width, height), output);

	delete [] input;

	return image;

	#endif
}



bool CImageLoaderJPG::saveImage(ox::io::IWriteFile* file, unsigned char* data, int width, int height, int channels)
{
    if (!data || !file || channels != 3)
        return false;

    jpeg_compress_struct compressionInfo;
    jpeg_error_mgr errorManager;
    JSAMPROW rowPointer[1];

    compressionInfo.err = jpeg_std_error(&errorManager);
    jpeg_create_compress(&compressionInfo);
    compressionInfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&compressionInfo);
    jpeg_set_quality(&compressionInfo, 100, TRUE);
    compressionInfo.image_width = width;
    compressionInfo.image_height = height;
    compressionInfo.input_components = 3;
    jpeg_default_colorspace(&compressionInfo);
    jpeg_stdio_dest(&compressionInfo, file->getStdioFile());
    jpeg_start_compress(&compressionInfo, TRUE);

    while (compressionInfo.next_scanline < compressionInfo.image_height)
    {
        rowPointer[0] = &data[compressionInfo.next_scanline * (width * 3)];
        jpeg_write_scanlines(&compressionInfo, rowPointer, 1);
    }

    jpeg_finish_compress(&compressionInfo);
    jpeg_destroy_compress(&compressionInfo);
    return true;
}


//! creates a loader which is able to load jpeg images
IImageLoader* createImageLoaderJPG()
{
	return new CImageLoaderJPG();
}

} // end namespace video
} // end namespace daisy
