// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImageLoaderPSD.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_IMAGE_LOADER_PSD_H_INCLUDED__
#define __C_IMAGE_LOADER_PSD_H_INCLUDED__

#include "ox/video/IImageLoader.h"
#include "ox/video/IImage.h"
#include "ox/video/IImageLoader.h"
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

	struct PsdHeader
	{
		char signature [4];	// Always equal to 8BPS.
		unsigned short version;		// Always equal to 1
		char reserved [6];	// Must be zero
		unsigned short channels;	// Number of any channels inc. alphas
		unsigned int height;		// Rows Height of image in pixel
		unsigned int width;		// Colums Width of image in pixel
		unsigned short depth;		// Bits/channel
		unsigned short mode;		// Color mode of the file (Bitmap/Grayscale..)
	} PACK_STRUCT;


// Default alignment
#ifdef _MSC_VER
#	pragma pack( pop, packing )
#endif

#undef PACK_STRUCT

/*!
	Surface Loader for psd images
*/
class CImageLoaderPSD : public IImageLoader
{
public:

	//! constructor
	CImageLoaderPSD();

	//! destructor
	virtual ~CImageLoaderPSD();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".tga")
	virtual bool isALoadableFileExtension(const char* fileName);

	//! returns true if the file maybe is able to be loaded by this class
	virtual bool isALoadableFileFormat(ox::io::IReadFile* file);

	//! creates a surface from the file
	virtual IImage* loadImage(ox::io::IReadFile* file);

private:

	bool readRawImageData(ox::io::IReadFile* file);
	bool readRLEImageData(ox::io::IReadFile* file);
	char getShiftFromChannel(char channelNr); 

	inline unsigned int convert2le (unsigned int value)
	{
		value = (value >> 16) | (value << 16);
		value = ((value >> 8) & 0xFF00FF) | ((value << 8) & 0xFF00FF00);
		return value;
	}

	inline unsigned short convert2le (unsigned short value)
	{
		value = (value >> 8) | (value << 8);
		return value;
	}

	// member variables

	PsdHeader header;
	unsigned int* imageData;
	bool error;

};


} // end namespace video
} // end namespace daisy


#endif

