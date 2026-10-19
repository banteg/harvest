#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/IImage.h"
#include "ox/video/IImageLoader.h"
#include "ox/io/IReadFile.h"
#include "ox/event/ILogger.h"
#include "CImageLoaderTGA.h"
#include <string.h>
#include "daisy/os.h"
#include "CColorConverter.h"
#include "CImage.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;


//! constructor
CImageLoaderTGA::CImageLoaderTGA()
{
}



//! destructor
CImageLoaderTGA::~CImageLoaderTGA()
{
}



//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderTGA::isALoadableFileExtension(const char* fileName)
{
	return strstr(fileName, ".tga") != 0;
}


//! loads a compressed tga. 
char *CImageLoaderTGA::loadCompressedImage(ox::io::IReadFile *file, STGAHeader *header)
{
	// This was written and sent in by Jon Pry, thank you very much!
	// I only changed the formatting a little bit.

	int bytesPerPixel = header->PixelDepth/8;
	int imageSize =  header->ImageHeight * header->ImageWidth * bytesPerPixel;
	char* data = new char[imageSize];
	int currentByte = 0;

	while(currentByte < imageSize)					
	{
		unsigned char chunkheader = 0;
		file->read(&chunkheader, sizeof(char)); // Read The Chunk's Header

		if(chunkheader < 128) // If The Chunk Is ox::core::A 'RAW' Chunk
		{
			chunkheader++; // Add 1 To The Value To Get Total Number Of Raw Pixels

			file->read(&data[currentByte], bytesPerPixel * chunkheader);
			currentByte += bytesPerPixel * chunkheader;
		}
		else
		{
			// If It's An RLE Header
			chunkheader -= 127;			// Subtract 127 To Get Rid Of The ID Bit
			file->read(&data[currentByte], bytesPerPixel);

			for(int counter = 0; counter < chunkheader; counter++)
			{
				for(int elementCounter=0; elementCounter < bytesPerPixel; elementCounter++)
					data[currentByte + elementCounter] =
						data[currentByte + elementCounter - bytesPerPixel];

				currentByte += bytesPerPixel;
			}
		}

	}

	return data;
}



//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderTGA::isALoadableFileFormat(ox::io::IReadFile* file)
{
	if (!file)
		return false;

	unsigned char type[3];
	file->read(&type, sizeof(unsigned char)*3);
	return (type[2]==2); // we currently only handle tgas of type 2.
}



//! creates a surface from the file
IImage* CImageLoaderTGA::loadImage(ox::io::IReadFile* file)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! creates a loader which is able to load tgas
IImageLoader* createImageLoaderTGA()
{
	return new CImageLoaderTGA();
}


} // end namespace video
} // end namespace daisy

