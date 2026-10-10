#include <iostream>
#include "CImageLoaderPSD.h"
#include "ox/core/CDimension2d.h"
#include "ox/video/IImage.h"
#include "ox/video/IImageLoader.h"
#include "ox/io/IReadFile.h"
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
CImageLoaderPSD::CImageLoaderPSD()
: imageData(0)
{
}



//! destructor
CImageLoaderPSD::~CImageLoaderPSD()
{
	delete [] imageData;
}



//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderPSD::isALoadableFileExtension(const char* fileName)
{
	return strstr(fileName, ".psd") != 0;
}



//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderPSD::isALoadableFileFormat(ox::io::IReadFile* file)
{
	if (!file)
		return false;

	unsigned char type[3];
	file->read(&type, sizeof(unsigned char)*3);
	return (type[2]==2); // we currently only handle tgas of type 2.
}



//! creates a surface from the file
IImage* CImageLoaderPSD::loadImage(ox::io::IReadFile* file)
{
	delete [] imageData;
	imageData = 0;

	file->seek(0);
	file->read(&header, sizeof(PsdHeader));

	header.version = convert2le(header.version);
	header.channels = convert2le(header.channels);
	header.height = convert2le(header.height);
	header.width = convert2le(header.width);
	header.depth = convert2le(header.depth);
	header.mode = convert2le(header.mode);

	if (header.signature[0] != '8' ||
		header.signature[1] != 'B' ||
		header.signature[2] != 'P' ||
		header.signature[3] != 'S')
		return 0;

	if (header.version != 1)
	{
		os::Printer::log("Unsupported PSD file version", file->getFileName(), ox::event::ELL_ERROR);
		return 0;
	}

	if (header.mode != 3 || header.depth != 8)
	{
		os::Printer::log("Unsupported PSD color mode or depth.\n", file->getFileName(), ox::event::ELL_ERROR);
		return 0;
	}

	// skip color mode data

	unsigned int l;
	file->read(&l, sizeof(int));
	l = convert2le(l);
	if (!file->seek(l, true))
	{
		os::Printer::log("Error seeking file pos to image resources.\n", file->getFileName(), ox::event::ELL_ERROR);
		return 0;
	}

	// skip image resources

	file->read(&l, sizeof(int));
	l = convert2le(l);
	if (!file->seek(l, true))
	{
		os::Printer::log("Error seeking file pos to layer and mask.\n", file->getFileName(), ox::event::ELL_ERROR);
		return 0;
	}

	// skip layer & mask

	file->read(&l, sizeof(int));
	l = convert2le(l);
	if (!file->seek(l, true))
	{
		os::Printer::log("Error seeking file pos to image data section.\n", file->getFileName(), ox::event::ELL_ERROR);
		return 0;
	}

	// read image data

	unsigned short compressionType;
	file->read(&compressionType, sizeof(unsigned short));
	compressionType = convert2le(compressionType);

	if (compressionType != 1 && compressionType != 0)
	{
		os::Printer::log("Unsupported psd compression mode.\n", file->getFileName(), ox::event::ELL_ERROR);
		return 0;
	}

	// create image data block

	imageData = new unsigned int[header.width * header.height];

	bool res = false;

	if (compressionType == 0)
		res = readRawImageData(file);	// RAW image data
	else
		res = readRLEImageData(file); // RLE compressed data

	ox::video::IImage* image = 0;

	if (res)
	{
		// create surface
		image = new CImage(ECF_A8R8G8B8,
			ox::core::CDimension2d<int>(header.width, header.height), imageData);
	}

	if (!image)
		delete [] imageData;
	imageData = 0;

	return image;
}



bool CImageLoaderPSD::readRawImageData(ox::io::IReadFile* file)
{
	unsigned char* tmpData = new unsigned char[header.width * header.height];

	for (int channel=0; channel<header.channels && channel < 3; ++channel)
	{
		if (!file->read(tmpData, sizeof(char) * header.width * header.height))
		{
			os::Printer::log("Error reading color channel\n", file->getFileName(), ox::event::ELL_ERROR);
			break;
		}

		char shift = getShiftFromChannel(channel);
		if (shift != -1)
		{
			unsigned int mask = 0xff << shift;

			for (unsigned int x=0; x<header.width; ++x)
				for (unsigned int y=0; y<header.height; ++y)
				{
					int index = x + y*header.width;
					imageData[index] = ~(~imageData[index] | mask);
					imageData[index] |= tmpData[index] << shift;
				}
		}

	}
		
	delete [] tmpData;
	return true;
}


bool CImageLoaderPSD::readRLEImageData(ox::io::IReadFile* file)
{
	/*	If the compression code is 1, the image data
		starts with the byte counts for all the scan lines in the channel
		(LayerBottom LayerTop), with each count stored as a two
		byte value. The RLE compressed data follows, with each scan line
		compressed separately. The RLE compression is the same compres-sion
		algorithm used by the Macintosh ROM routine PackBits, and
		the TIFF standard.
		If the Layers Size, and therefore the data, is odd, a pad byte will
		be inserted at the end of the row.
	*/

	/*
	A pseudo code fragment to unpack might look like this:

	Loop until you get the number of unpacked bytes you are expecting:
		Read the next source byte into n.
		If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
		Else if n is between -127 and -1 inclusive, copy the next byte -n+1
		times.
		Else if n is -128, noop.
	Endloop

	In the inverse routine, it is best to encode a 2-byte repeat run as a replicate run
	except when preceded and followed by a literal run. In that case, it is best to merge
	the three runs into one literal run. Always encode 3-byte repeats as replicate runs.
	That is the essence of the algorithm. Here are some additional rules:
	- Pack each row separately. Do not compress across row boundaries.
	- The number of uncompressed bytes per row is defined to be (ImageWidth + 7)
	/ 8. If the uncompressed bitmap is required to have an even number of bytes per
	row, decompress into word-aligned buffers.
	- If a run is larger than 128 bytes, encode the remainder of the run as one or more
	additional replicate runs.
	When PackBits data is decompressed, the result should be interpreted as per com-pression
	type 1 (no compression).
	*/

	unsigned char* tmpData = new unsigned char[header.width * header.height];
	unsigned short *rleCount= new unsigned short [header.height * header.channels];

	int size=0;      
      
	for (unsigned int y=0; y<header.height * header.channels; ++y)
	{
		if (!file->read(&rleCount[y], sizeof(unsigned short)))
		{
			delete [] tmpData;
			delete [] rleCount;
			os::Printer::log("Error reading rle rows\n", file->getFileName(), ox::event::ELL_ERROR);
			return false;
		}

		rleCount[y] = convert2le (rleCount[y]);
		size += rleCount[y];
	}

	char *buf = new char[size];       
	if (!file->read(buf, size))
	{
		delete [] rleCount;
		delete [] buf;
		delete [] tmpData;
		os::Printer::log("Error reading rle rows\n", file->getFileName(), ox::event::ELL_ERROR);
		return false;
	}

	unsigned short *rcount=rleCount; 
	
	char rh;
	unsigned short bytesRead;
	unsigned char *dest;
	char *pBuf = buf;

	// decompress packbit rle 

	for (int channel=0; channel<header.channels; channel++)
	{
		for (unsigned int y=0; y<header.height; ++y, ++rcount) 
		{
			bytesRead=0;
			dest = &tmpData[y*header.width];
			
			while (bytesRead < *rcount)
			{
				rh = *pBuf++;
				++bytesRead;
				
				if (rh >= 0)
				{
					++rh;

					while (rh--)
					{ 
						*dest = *pBuf++;
						++bytesRead;
						++dest;
					} 
				}
				else 
				if (rh > -128)
				{
					rh = -rh +1;

					while (rh--)
					{
						*dest = *pBuf;
						++dest;
					}

					++pBuf;
					++bytesRead;
				}
			}
		}
		
		char shift = getShiftFromChannel(channel);

		if (shift != -1)
		{
			unsigned int mask = 0xff << shift;

			for (unsigned int x=0; x<header.width; ++x)
				for (unsigned int y=0; y<header.height; ++y)
				{
					int index = x + y*header.width;
					imageData[index] = ~(~imageData[index] | mask);
					imageData[index] |= tmpData[index] << shift;
				}
		}
	}
       
	delete [] rleCount;
	delete [] buf;
	delete [] tmpData;

	return true;
}


char CImageLoaderPSD::getShiftFromChannel(char channelNr)
{
	switch(channelNr)
	{
	case 0:
		return 16;  // red
	case 1:
		return 8;   // green
	case 2:
		return 0;   // blue
	case 3:
		return header.channels == 4 ? 24 : -1;	// ?
	case 4:
		return 24;  // alpha
	default:
		return -1;
	}
}



//! creates a loader which is able to load tgas
IImageLoader* createImageLoaderPSD()
{
	return new CImageLoaderPSD();
}


} // end namespace video
} // end namespace daisy

