// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImageLoaderBmp.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/IImage.h"
#include "ox/video/IImageLoader.h"
#include "ox/io/IReadFile.h"
#include "ox/event/ILogger.h"
#include "CImageLoaderBmp.h"
#include <string.h>
#include "ox/video/SColor.h"
#include "ox/video/ColorPacking.h"
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
CImageLoaderBmp::CImageLoaderBmp()
: BmpData(0), PaletteData(0)
{
}



//! destructor
CImageLoaderBmp::~CImageLoaderBmp()
{
	if (PaletteData)
		delete [] PaletteData;

	if (BmpData)
		delete [] BmpData;
}



//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderBmp::isALoadableFileExtension(const char* fileName)
{
	return strstr(fileName, ".bmp") != 0;
}



//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderBmp::isALoadableFileFormat(ox::io::IReadFile* file)
{
	unsigned short headerID;
	file->read(&headerID, sizeof(unsigned short));
	return headerID == 0x4d42;
}


void CImageLoaderBmp::decompress8BitRLE(char*& BmpData, int size, int width, int height, int pitch)
{
	char* p = BmpData;
	char* newBmp = new char[(width+pitch)*height];
	char* d = newBmp;
	char* destEnd = newBmp + (width+pitch)*height;
	int line = 0;

	while (BmpData - p < size && d < destEnd)
	{
		if (*p == 0)
		{
			++p;

			switch(*p)
			{
			case 0: // end of line
				++p; 
				++line;
				d = newBmp + (line*(width+pitch));
				break; 
			case 1: // end of bmp
				delete [] BmpData;
				BmpData = newBmp;
				return; 
			case 2:
				++p; d +=(unsigned char)*p;  // delta
				++p; d += ((unsigned char)*p)*(width+pitch); 
				++p;
				break;
			default:
				{
					// absolute mode
					int count = (unsigned char)*p; ++p;
					int readAdditional = ((2-(count%2))%2);
					int i;

					for (i=0; i<count; ++i)
					{
						*d = *p;
						++p;
						++d;
					}

					for (i=0; i<readAdditional; ++i)
						++p;
				}
			}
		}
		else
		{
			int count = (unsigned char)*p; ++p;			
			int color = (unsigned char)*p; ++p;
			for (int i=0; i<count; ++i)
			{
				*d = color;
				++d;
			}
		}
	}

	delete [] BmpData;
	BmpData = newBmp;
}


void CImageLoaderBmp::decompress4BitRLE(char*& BmpData, int size, int width, int height, int pitch)
{
	int lineWidth = (width+1)/2+pitch;
	char* p = BmpData;
	char* newBmp = new char[lineWidth*height];
	char* d = newBmp;
	char* destEnd = newBmp + lineWidth*height;
	int line = 0;
	int shift = 4;

	while (BmpData - p < size && d < destEnd)
	{
		if (*p == 0)
		{
			++p;

			switch(*p)
			{
			case 0: // end of line
				++p; 
				++line;
				d = newBmp + (line*lineWidth);
				shift = 4;
				break; 
			case 1: // end of bmp
				delete [] BmpData;
				BmpData = newBmp;
				return; 
			case 2:
				{
					++p; 
					int x = (unsigned char)*p; ++p;
					int y = (unsigned char)*p; ++p;
					d += x/2 + y*lineWidth;
					shift = x%2==0 ? 4 : 0;
				}
				break;
			default:
				{
					// absolute mode
					int count = (unsigned char)*p; ++p;
					int readAdditional = ((2-((count)%2))%2);
					int readShift = 4;
					int i;

					for (i=0; i<count; ++i)
					{
						int color = (((unsigned char)*p) >> readShift) & 0x0f;
						readShift -= 4;
						if (readShift < 0)
						{
							++*p;
							readShift = 4;
						}

						char mask = 0x0f << shift;
						*d = (*d & (~mask)) | ((color << shift) & mask);

						shift -= 4;
						if (shift < 0)
						{
							shift = 4;
							++d;
						}

					}

					for (i=0; i<readAdditional; ++i)
						++p;
				}
			}
		}
		else
		{
			int count = (unsigned char)*p; ++p;			
			int color1 = (unsigned char)*p; color1 = color1 & 0x0f;
			int color2 = (unsigned char)*p; color2 = (color2 >> 4) & 0x0f;
			++p;

			for (int i=0; i<count; ++i)
			{
				char mask = 0x0f << shift;
				char toSet = (shift==0 ? color1 : color2) << shift;
				*d = (*d & (~mask)) | (toSet & mask);

				shift -= 4;
				if (shift < 0)
				{
					shift = 4;
					++d;
				}
			}
		}
	}

	delete [] BmpData;
	BmpData = newBmp;
}



//! creates a surface from the file
IImage* CImageLoaderBmp::loadImage(ox::io::IReadFile* file)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! creates a loader which is able to load windows bitmaps
IImageLoader* createImageLoaderBmp()
{
	return new CImageLoaderBmp;
}



} // end namespace video
} // end namespace daisy
