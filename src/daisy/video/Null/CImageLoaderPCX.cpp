// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImageLoaderPCX.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/IImage.h"
#include "ox/video/IImageLoader.h"
#include "ox/io/IReadFile.h"
#include "ox/event/ILogger.h"
#include "CImageLoaderPCX.h"
#include "ox/video/SColor.h"
#include "ox/video/ColorPacking.h"
#include <string.h>
#include "CColorConverter.h"
#include "CImage.h"
#include "daisy/os.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;


//! constructor
CImageLoaderPCX::CImageLoaderPCX()
: PCXData(0), PaletteData(0)
{
}



//! destructor
CImageLoaderPCX::~CImageLoaderPCX()
{
	if (PaletteData)
		delete [] PaletteData;

	if (PCXData)
		delete [] PCXData;
}



//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderPCX::isALoadableFileExtension(const char* fileName)
{
	return (strstr(fileName, ".PCX") != 0) || (strstr(fileName, ".pcx") != 0);
}



//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderPCX::isALoadableFileFormat(ox::io::IReadFile* file)
{
	unsigned char headerID;
	file->read(&headerID, sizeof(headerID));
	return headerID == 0x0a;
}


//! creates a image from the file
IImage* CImageLoaderPCX::loadImage(ox::io::IReadFile* file)
{
	SPCXHeader header;

	file->seek(0);
	file->read(&header, sizeof(header));

	int pitch = 0;

	//! return if the header is wrong
	if (header.Manufacturer != 0x0a && header.Encoding != 0x01)
		return 0;

	// return if this isn't a supported type
	if( (header.BitsPerPixel < 8) || (header.BitsPerPixel > 24) )
	{
		os::Printer::log("Unsupported bits per pixel in PCX file.",
			file->getFileName(), ox::event::ELL_WARNING);
		return 0;
	}

	// read palette
	if( header.BitsPerPixel >= 8 )
	{
		int pos = file->getPos();
		unsigned char palIndicator;
		file->seek( file->getSize()-256*3-1, false );

		file->read( &palIndicator, 1 );

		if( palIndicator != 12 )
		{
			os::Printer::log("Unsupported pal indicator in PCX file.",
				file->getFileName(), ox::event::ELL_WARNING);
			return 0;
		}

		unsigned char *tempPalette = new unsigned char[768];
		PaletteData = new int[256];
		file->read( tempPalette, 768 );

		for( int i=0; i<256; i++ )
		{
			PaletteData[i] = (tempPalette[i*3+0] << 16) |
							 (tempPalette[i*3+1] << 8) |
							 (tempPalette[i*3+2] );
		}

		delete [] tempPalette;

		file->seek( pos );
	}
	else if( header.BitsPerPixel == 4 )
	{
		PaletteData = new int[16];
		for( int i=0; i<256; i++ )
		{
			PaletteData[i] = (header.Palette[i*3+0] << 16) |
							 (header.Palette[i*3+1] << 8) |
							 (header.Palette[i*3+2]);
		}
	}

	// read image data
	int width, height, imagebytes;
	width = header.XMax - header.XMin + 1;
	height = header.XMax - header.XMin + 1;
	imagebytes = header.BytesPerLine * height * header.Planes * header.BitsPerPixel / 8;
	PCXData = new char[imagebytes];

	char cnt, value;
	for( int offset = 0; offset < imagebytes; )
	{
		file->read( &cnt, 1 );
		if( !((cnt & 0xc0) == 0xc0) )
		{
			value = cnt;
			cnt = 1;
		}
		else
		{
			cnt &= 0x3f;
			file->read( &value, 1 );
		}
		while( cnt )
		{
			PCXData[offset] = value;
			cnt--;
			offset++;
		}
	}

	// create image
	ox::video::IImage* image = 0;
	pitch = header.BytesPerLine - width * header.Planes * header.BitsPerPixel / 8;

	if (pitch < 0)
		pitch = -pitch;

	switch(header.BitsPerPixel)
	{
	case 8:
		image = new CImage(ECF_A1R5G5B5, ox::core::CDimension2d<int>(width, height));
		CColorConverter::convert8BitTo16Bit(PCXData, (short*)image->lock(), width, height, pitch, PaletteData);
		image->unlock();
		break;
	case 24:
		image = new CImage(ECF_R8G8B8, ox::core::CDimension2d<int>(width, height));
		CColorConverter::convert24BitTo24BitFlipMirrorColorShuffle(PCXData, (char*)image->lock(), width, height, pitch);
		image->unlock();
		break;
	};

	// clean up

	if( PaletteData )
		delete [] PaletteData;
	PaletteData = 0;

	if( PCXData )
		delete [] PCXData;
	PCXData = 0;

	return image;
}


//! creates a loader which is able to load tgas
IImageLoader* createImageLoaderPCX()
{
	return new CImageLoaderPCX();
}



} // end namespace video
} // end namespace daisy
