// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImageLoaderPCX.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/IImage.h"
#include "ox/video/IImageLoader.h"
#include "ox/io/IReadFile.h"
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
    __builtin_trap();
}


//! creates a loader which is able to load tgas
IImageLoader* createImageLoaderPCX()
{
	return new CImageLoaderPCX();
}



} // end namespace video
} // end namespace daisy
