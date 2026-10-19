// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImageLoaderTGA.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_IMAGE_LOADER_TGA_H_INCLUDED__
#define __C_IMAGE_LOADER_TGA_H_INCLUDED__

#include "ox/video/IImageLoader.h"
#include "ox/video/IImage.h"
#include "ox/io/IReadFile.h"


namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;


// byte-align structures
#ifdef _MSC_VER
#	pragma pack( push, packing )
#	pragma pack( 1 )
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error compiler not supported
#endif

	 struct STGAHeader{
	    unsigned char IdLength;
	    unsigned char ColorMapType;
	    unsigned char ImageType;
	    unsigned char FirstEntryIndex[2];
	    unsigned short ColorMapLength;
	    unsigned char ColorMapEntrySize;
	    unsigned char XOrigin[2];
	    unsigned char YOrigin[2];
	    unsigned short ImageWidth;
	    unsigned short ImageHeight;
	    unsigned char PixelDepth;
	    unsigned char ImageDescriptor;
    } PACK_STRUCT;


// Default alignment
#ifdef _MSC_VER
#	pragma pack( pop, packing )
#endif

#undef PACK_STRUCT

/*!
	Surface Loader for targa images
*/
class CImageLoaderTGA : public IImageLoader
{
public:

	//! constructor
	CImageLoaderTGA();

	//! destructor
	virtual ~CImageLoaderTGA();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".tga")
	virtual bool isALoadableFileExtension(const char* fileName);

	//! returns true if the file maybe is able to be loaded by this class
	virtual bool isALoadableFileFormat(ox::io::IReadFile* file);

	//! creates a surface from the file
	virtual IImage* loadImage(ox::io::IReadFile* file);

private:

	//! loads a compressed tga. Was written and sent in by Jon Pry, thank you very much!
	char* loadCompressedImage(ox::io::IReadFile *file, STGAHeader *header);

};


} // end namespace video
} // end namespace daisy


#endif

