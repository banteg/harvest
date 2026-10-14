// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImageLoaderPCX.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_IMAGE_LOADER_PCX_H_INCLUDED__
#define __C_IMAGE_LOADER_PCX_H_INCLUDED__

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

	struct SPCXHeader
	{
		unsigned char	Manufacturer;
		unsigned char	Version;
		unsigned char	Encoding;
		unsigned char	BitsPerPixel;
		unsigned short	XMin;
		unsigned short	YMin;
		unsigned short	XMax;
		unsigned short	YMax;
		unsigned short HorizDPI;
		unsigned short	VertDPI;
		unsigned char	Palette[48];
		unsigned char	Reserved;
		unsigned char	Planes;
		unsigned short	BytesPerLine;
		unsigned short	PaletteType;
		unsigned short	HScrSize;
		unsigned short	VScrSize;
		unsigned char	Filler[54];
	} PACK_STRUCT;


// Default alignment
#ifdef _MSC_VER
#	pragma pack( pop, packing )
#endif

#undef PACK_STRUCT


/*!
	Image Loader for Windows PCX bitmaps.
	This loader was written and sent in by Dean P. Macri. I modified
	only some small bits of it.
*/
class CImageLoaderPCX : public IImageLoader
{
public:

	//! constructor
	CImageLoaderPCX();

	//! destructor
	virtual ~CImageLoaderPCX();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".tga")
	virtual bool isALoadableFileExtension(const char* fileName);

	//! returns true if the file maybe is able to be loaded by this class
	virtual bool isALoadableFileFormat(ox::io::IReadFile* file);

	//! creates a surface from the file
	virtual IImage* loadImage(ox::io::IReadFile* file);

private:

	char* PCXData;
	int* PaletteData;
};


} // end namespace video
} // end namespace daisy


#endif

