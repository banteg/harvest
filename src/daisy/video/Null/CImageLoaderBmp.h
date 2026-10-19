// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImageLoaderBmp.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_IMAGE_LOADER_BMP_H_INCLUDED__
#define __C_IMAGE_LOADER_BMP_H_INCLUDED__

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

	struct SBMPHeader
	{
		unsigned short	Id;					//	BM - Windows 3.1x, 95, NT, 98, 2000, ME, XP
											//	BA - OS/2 Bitmap Array
											//	CI - OS/2 Color Icon
											//	CP - OS/2 Color Pointer
											//	IC - OS/2 Icon
											//	PT - OS/2 Pointer
		unsigned int	FileSize;
		unsigned int	Reserved;
		unsigned int	BitmapDataOffset;
		unsigned int	BitmapHeaderSize;	// should be 28h for windows bitmaps or
											// 0Ch for OS/2 1.x or F0h for OS/2 2.x
		unsigned int Width;
		unsigned int Height;
		unsigned short Planes;
		unsigned short BPP;					// 1: Monochrome bitmap
											// 4: 16 color bitmap
											// 8: 256 color bitmap
											// 16: 16bit (high color) bitmap
											// 24: 24bit (true color) bitmap
											// 32: 32bit (true color) bitmap

		unsigned int  Compression;			// 0: none (Also identified by BI_RGB)
											// 1: RLE 8-bit / pixel (Also identified by BI_RLE4)
											// 2: RLE 4-bit / pixel (Also identified by BI_RLE8)
											// 3: Bitfields  (Also identified by BI_BITFIELDS)

		unsigned int  BitmapDataSize;		// Size of the bitmap data in bytes. This number must be rounded to the next 4 byte boundary.
		unsigned int  PixelPerMeterX;
		unsigned int  PixelPerMeterY;
		unsigned int  Colors;
		unsigned int  ImportantColors;
	} PACK_STRUCT;


// Default alignment
#ifdef _MSC_VER
#	pragma pack( pop, packing )
#endif

#undef PACK_STRUCT


/*!
	Surface Loader fow Windows bitmaps
*/
class CImageLoaderBmp : public IImageLoader
{
public:

	//! constructor
	CImageLoaderBmp();

	//! destructor
	virtual ~CImageLoaderBmp();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".tga")
	virtual bool isALoadableFileExtension(const char* fileName);

	//! returns true if the file maybe is able to be loaded by this class
	virtual bool isALoadableFileFormat(ox::io::IReadFile* file);

	//! creates a surface from the file
	virtual IImage* loadImage(ox::io::IReadFile* file);

private:

	void decompress8BitRLE(char*& BmpData, int size, int width, int height, int pitch);

	void decompress4BitRLE(char*& BmpData, int size, int width, int height, int pitch);

	char* BmpData;
	int* PaletteData;

};


} // end namespace video
} // end namespace daisy


#endif
