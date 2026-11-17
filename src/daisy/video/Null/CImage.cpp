// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImage.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/ColorPacking.h"
#include "ox/video/IImage.h"
#include "ox/video/SColor.h"
#include "ox/event/ILogger.h"
#include "CImage.h"
#include "daisy/os.h"
#include <memory.h>

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;


//! constructor
CImage::CImage(ECOLOR_FORMAT format, const ox::core::CDimension2d<int>& size)
: Format(format), Size(size), Data(0)
{
	initData();
}


//! constructor
CImage::CImage(ECOLOR_FORMAT format, const ox::core::CDimension2d<int>& size, void* data)
: Format(format), Size(size)
{
	Data = data;
	initData();
}




//! constructor
CImage::CImage(ECOLOR_FORMAT format, IImage* imageToCopy)
: Format(format), Data(0)
{
	if (!imageToCopy)
		return;

	Size = imageToCopy->getDimension();
	initData();

	// now copy data from other image

	if (Format == (ECOLOR_FORMAT)imageToCopy->getColorFormat())
	{
		memcpy(Data, imageToCopy->lock(), imageToCopy->getImageDataSizeInBytes());
		imageToCopy->unlock();
	}
	else
	if (Format == ox::video::ECF_A1R5G5B5)
		for (int x=0; x<(int)Size.Width; ++x)
			for (int y=0; y<(int)Size.Height; ++y)
				((short*)Data)[y*Size.Width + x] = ox::video::toA1R5G5B5(imageToCopy->getPixel(x,y).color);
	else
		os::Printer::log("CImage: Unsupported format to copy from.", ox::event::ELL_ERROR);
}



//! constructor
CImage::CImage(IImage* imageToCopy,	const ox::core::CPosition2d<int>& pos,
			   const ox::core::CDimension2d<int>& size)
 : Data(0), Size(0,0)
{
	if (!imageToCopy)
		return;

	Format = (ECOLOR_FORMAT)imageToCopy->getColorFormat();
	Size = size;

	initData();

	// copy image part
	char* data = (char*)Data;
	char* source = (char*)imageToCopy->lock();
	int bytes = imageToCopy->getBytesPerPixel();
	int srcpitch = imageToCopy->getDimension().Width * bytes;
	int tgtpitch = Size.Width*bytes;

	for (int x=0; x<size.Width; ++x)
		for (int y=0; y<size.Height; ++y)
		{
			for (int b=0; b<bytes; ++b)
				data[y*tgtpitch + (x*bytes) + b] =
					source[(y+pos.Y)*srcpitch + ((pos.X+x)*bytes) + b];
		}

	imageToCopy->unlock();
}




//! assumes format and size has been set and creates the rest
void CImage::initData()
{
	BitsPerPixel = getBitsPerPixelFromFormat();
	BytesPerPixel = BitsPerPixel / 8;

	if (!Data)
		Data = new char[Size.Height * Size.Width * BytesPerPixel];
}


//! destructor
CImage::~CImage()
{
	delete [] (char*)Data;
}



//! Lock function.
void* CImage::lock()
{
	return Data;
}



//! Unlock function.
void CImage::unlock()
{
}



//! Returns width and height of image data.
const ox::core::CDimension2d<int>& CImage::getDimension()
{
	return Size;
}



//! Returns bits per pixel.
int CImage::getBitsPerPixel()
{
	return BitsPerPixel;
}


//! Returns bytes per pixel
int CImage::getBytesPerPixel()
{
	return BytesPerPixel;
}



//! Returns image data size in bytes
int CImage::getImageDataSizeInBytes()
{
	return BytesPerPixel * Size.Width * Size.Height;
}



//! Returns image data size in pixels
int CImage::getImageDataSizeInPixels()
{
	return Size.Width * Size.Height;
}



//! returns mask for red value of a pixel
unsigned int CImage::getRedMask()
{
	return RedMask;
}



//! returns mask for green value of a pixel
unsigned int CImage::getGreenMask()
{
	return GreenMask;
}



//! returns mask for blue value of a pixel
unsigned int CImage::getBlueMask()
{
	return BlueMask;
}



//! returns mask for alpha value of a pixel
unsigned int CImage::getAlphaMask()
{
	return AlphaMask;
}


int CImage::getBitsPerPixelFromFormat()
{
	switch(Format)
	{
	case ECF_A1R5G5B5:
		AlphaMask = 0x1<<15;
		RedMask = 0x1F<<10;
		GreenMask = 0x1F<<5;
		BlueMask = 0x1F;
		return 16;
	case ECF_R5G6B5:
		AlphaMask = 0x0;
		RedMask = 0x1F<<11;
		GreenMask = 0x3F<<5;
		BlueMask = 0x1F;
		return 16;
	case ECF_R8G8B8:
		AlphaMask = 0x0;
		RedMask = 0xFF<<16;
		GreenMask = 0xFF<<8;
		BlueMask = 0xFF;
		return 24;
	case ECF_A8R8G8B8:
		AlphaMask = 0xFF<<24;
		RedMask = 0xFF<<16;
		GreenMask = 0xFF<<8;
		BlueMask = 0xFF;
		return 32;
	}

	os::Printer::log("CImage: Unknown color format.", ox::event::ELL_ERROR);
	return 0;
}


//! returns a pixel
SColor CImage::getPixel(int x, int y)
{
	if (x < 0 || y < 0 || x >= Size.Width || y >= Size.Height)
		return SColor(0);

	switch(Format)
	{
	case ECF_A1R5G5B5:
		{
			short color = ((short*)Data)[y*Size.Width + x];
			if (color < 0)
				return SColor(0xff000000 |
					(color << 9 & 0xf80000) |
					(color << 6 & 0xf800) |
					(color << 3 & 0xff));
			return SColor(
				(color << 9 & 0xf80000) |
				(color << 6 & 0xf800) |
				(color << 3 & 0xff));
		}
	case ECF_R5G6B5:
		return R5G6B5toA8R8G8B8(((short*)Data)[y*Size.Width + x]);
	case ECF_A8R8G8B8:
		return ((int*)Data)[y*Size.Width + x];
	case ECF_R8G8B8:
		{
			unsigned char* p = &((unsigned char*)Data)[(y*3)*Size.Width + (x*3)];
			return SColor(255,p[0],p[1],p[2]);
		}
	}

	return SColor(0);
}


//! returns the color format
int CImage::getColorFormat()
{
	return Format;
}


//! clips a x coordinate into the screen
inline void CImage::clipX(int &x)
{
	if (x<0)
		x = 0;
	else
		if (x > Size.Width)
			x = Size.Width;
}



//! clips a y coordinate into the screen
inline void CImage::clipY(int &y)
{
	if (y<0)
		y = 0;
	else
		if (y > Size.Height)
			y = Size.Height;
}



//! exchanges two ints
inline void CImage::exchange(int& a, int& b)
{
	int tmp;
	tmp = a;
	a = b;
	b = tmp;
}


//! draws a rectangle
void CImage::drawRectangle(const ox::core::CRect<int>& rect, SColor color)
{
	drawRectangle(	rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
					rect.LowerRightCorner.X, rect.LowerRightCorner.Y, color);
}


//! draws a rectangle
void CImage::drawRectangle(int x, int y, int x2, int y2, SColor color)
{
	if (Format != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}

	// clip

	clipX(x); clipX(x2);
	clipY(y); clipY(y2);

	// switch x and x2 if neccecary

	if (x > x2)
		exchange(x,x2);

	if (y > y2)
		exchange(y,y2);

	// draw

	int l=y*Size.Width;
	int ix;

	if (color.getAlpha()==255)
	{
		// quickly draw without alpha.

		short c = ox::video::toA1R5G5B5(color.color);

		for (int iy=y; iy<y2; ++iy)
		{
			for (ix=x; ix<x2; ++ix)
				((short*)Data)[l + ix] = c;

			l += Size.Width;
		}
	}
	else
	{
		// draw with alpha

		int ia = color.getAlpha();
		int a = 255-ia;

		int r = getRed(ox::video::toA1R5G5B5(color.color)) * ia;
		int g = getGreen(ox::video::toA1R5G5B5(color.color)) * ia;
		int b = getBlue(ox::video::toA1R5G5B5(color.color)) * ia;

		short *src;

		for (int iy=y; iy<y2; ++iy)
		{
			for (ix=x; ix<x2; ++ix)
			{
				src = &((short*)Data)[l + ix];
				*src =	ox::video::RGB16(
					(ox::video::getRed(*src)*a + r)>>5,
					(ox::video::getGreen(*src)*a + g)>>5,
					(ox::video::getBlue(*src)*a + b)>>5);
			}

			l += Size.Width;
		}
	}
}



//! clips coordinates into the rect. returns false if the rects is completely outside
//! and does not need to be drawn.
inline bool CImage::clipRect(const int targetWidth, const int targetHeight, int& targetX, int& targetY, int& xInSource, int& yInSource, int& sourceWidth, int& soureHeight)
{
	if (targetX<0)
	{
		sourceWidth += targetX;
		if (sourceWidth <= 0)
			return false;

		xInSource -= targetX;
		targetX = 0;
	}

	if (targetX+sourceWidth>targetWidth)
	{
		sourceWidth -= (targetX + sourceWidth) - targetWidth;
		if (sourceWidth <= 0)
			return false;
	}

	if (targetY<0)
	{
		soureHeight += targetY;
		if (soureHeight <= 0)
			return false;

		yInSource -= targetY;
		targetY = 0;
	}

	if (targetY+soureHeight>targetHeight)
	{
		soureHeight -= (targetY + soureHeight) - targetHeight;
		if (soureHeight <= 0)
			return false;
	}

	return true;
}



//! copies this surface into another
void CImage::copyTo(CImage* target, const ox::core::CPosition2d<int>& pos)
{
	copyTo(target, pos.X, pos.Y);
}



//! copies this surface into another
void CImage::copyTo(CImage* target, int x, int y)
{
	if (Format != ECF_A1R5G5B5 ||
		target->getColorFormat() != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}

	short* data = (short*)target->lock();
	ox::core::CDimension2d<int> size = target->getDimension();

	// clip

	int ownWidth = Size.Width;
	int ownHeight = Size.Height;
	int ownX = 0;
	int ownY = 0;

	if (!clipRect(size.Width, size.Height, x, y, ownX, ownY, ownWidth, ownHeight))
		return;

	// copy

	int ltarget=y*size.Width + x;
	int lown=ownY*Size.Width + ownX;
	size_t dataRowSizeInBytes = ownWidth*sizeof(short);

	for (int iy=0; iy<ownHeight; ++iy)
	{
		memcpy(&data[ltarget], &((short*)Data)[lown], dataRowSizeInBytes);

		lown += Size.Width;
		ltarget += size.Width;
	}

	target->unlock();
}



//! copies this surface into another
void CImage::copyTo(CImage* target, const ox::core::CPosition2d<int>& pos, const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect)
{
	if (Format != ECF_A1R5G5B5 ||
		target->getColorFormat() != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}

	if (sourceRect.UpperLeftCorner.X >= sourceRect.LowerRightCorner.X ||
		sourceRect.UpperLeftCorner.Y >= sourceRect.LowerRightCorner.Y)
		return;

	ox::core::CPosition2d<int> targetPos = pos;
	ox::core::CPosition2d<int> sourcePos = sourceRect.UpperLeftCorner;
	ox::core::CDimension2d<int> sourceSize(sourceRect.getWidth(), sourceRect.getHeight());
	const ox::core::CDimension2d<int> targetSurfaceSize = target->getDimension();

	if (clipRect)
	{
		if (targetPos.X < clipRect->UpperLeftCorner.X)
		{
			sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
			if (sourceSize.Width <= 0)
				return;

			sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
			targetPos.X = clipRect->UpperLeftCorner.X;
		}

		if (targetPos.X + sourceSize.Width > clipRect->LowerRightCorner.X)
		{
			sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
			if (sourceSize.Width <= 0)
				return;
		}

		if (targetPos.Y < clipRect->UpperLeftCorner.Y)
		{
			sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
			if (sourceSize.Height <= 0)
				return;

			sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
			targetPos.Y = clipRect->UpperLeftCorner.Y;
		}

		if (targetPos.Y + sourceSize.Height > clipRect->LowerRightCorner.Y)
		{
			sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
			if (sourceSize.Height <= 0)
				return;
		}
	}

	// clip these coordinates

	if (targetPos.X<0)
	{
		sourceSize.Width += targetPos.X;
		if (sourceSize.Width <= 0)
			return;

		sourcePos.X -= targetPos.X;
		targetPos.X = 0;
	}

	if (targetPos.X + sourceSize.Width > targetSurfaceSize.Width)
	{
		sourceSize.Width -= (targetPos.X + sourceSize.Width) - targetSurfaceSize.Width;
		if (sourceSize.Width <= 0)
			return;
	}

	if (targetPos.Y<0)
	{
		sourceSize.Height += targetPos.Y;
		if (sourceSize.Height <= 0)
			return;

		sourcePos.Y -= targetPos.Y;
		targetPos.Y = 0;
	}

	if (targetPos.Y + sourceSize.Height > targetSurfaceSize.Height)
	{
		sourceSize.Height -= (targetPos.Y + sourceSize.Height) - targetSurfaceSize.Height;
		if (sourceSize.Height <= 0)
			return;
	}

	// draw everything

	short* targetData = (short*)target->lock();
	int ltarget = targetPos.Y * targetSurfaceSize.Width + targetPos.X;
	int lsource = sourcePos.Y * Size.Width + sourcePos.X;
	size_t dataRowSizeInBytes = sourceSize.Width * sizeof(short);

	for (int iy=0; iy<sourceSize.Height; ++iy)
	{
		memcpy(&targetData[ltarget], &((short*)Data)[lsource], dataRowSizeInBytes);
		lsource += Size.Width;
		ltarget += targetSurfaceSize.Width;
	}
}



//! copies this surface into another, using the alpha mask
void CImage::copyToWithAlpha(CImage* target, const ox::core::CPosition2d<int>& pos, const ox::core::CRect<int>& sourceRect)
{
	if (Format != ECF_A1R5G5B5 ||
		target->getColorFormat() != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}


	if (sourceRect.UpperLeftCorner.X >= sourceRect.LowerRightCorner.X ||
		sourceRect.UpperLeftCorner.Y >= sourceRect.LowerRightCorner.Y)
		return;

	ox::core::CPosition2d<int> targetPos = pos;
	ox::core::CPosition2d<int> sourcePos = sourceRect.UpperLeftCorner;
	ox::core::CDimension2d<int> sourceSize(sourceRect.getWidth(), sourceRect.getHeight());
	const ox::core::CDimension2d<int> targetSurfaceSize = target->getDimension();

	// clip these coordinates

	if (targetPos.X<0)
	{
		sourceSize.Width += targetPos.X;
		if (sourceSize.Width <= 0)
			return;

		sourcePos.X -= targetPos.X;
		targetPos.X = 0;
	}

	if (targetPos.X + sourceSize.Width > targetSurfaceSize.Width)
	{
		sourceSize.Width -= (targetPos.X + sourceSize.Width) - targetSurfaceSize.Width;
		if (sourceSize.Width <= 0)
			return;
	}

	if (targetPos.Y<0)
	{
		sourceSize.Height += targetPos.Y;
		if (sourceSize.Height <= 0)
			return;

		sourcePos.Y -= targetPos.Y;
		targetPos.Y = 0;
	}

	if (targetPos.Y + sourceSize.Height > targetSurfaceSize.Height)
	{
		sourceSize.Height -= (targetPos.Y + sourceSize.Height) - targetSurfaceSize.Height;
		if (sourceSize.Height <= 0)
			return;
	}

	// draw everything

	short* targetData = (short*)target->lock();
	int ltarget = targetPos.Y * targetSurfaceSize.Width + targetPos.X;
	int lsource = sourcePos.Y * Size.Width + sourcePos.X;
	size_t dataRowSizeInBytes = sourceSize.Width * sizeof(short);

	short alphaMask = getAlphaMask();
	short* p;
	short* end;
	short* t;

	for (int iy=0; iy<sourceSize.Height; ++iy)
	{
		p = &((short*)Data)[lsource];
		end = p + (dataRowSizeInBytes>>1);
		t = &targetData[ltarget];

		while(p != end)
		{
			if (*p & alphaMask)
				*t = *p;

			++t;
			++p;
		}
		lsource += Size.Width;
		ltarget += targetSurfaceSize.Width;
	}

}




//! copies this surface into another, using the alpha mask, an cliprect and a color to add with
void CImage::copyToWithAlpha(CImage* target, const ox::core::CPosition2d<int>& pos, const ox::core::CRect<int>& sourceRect, short color, const ox::core::CRect<int>* clipRect)
{
	if (Format != ECF_A1R5G5B5 ||
		target->getColorFormat() != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}

	if (sourceRect.UpperLeftCorner.X >= sourceRect.LowerRightCorner.X ||
		sourceRect.UpperLeftCorner.Y >= sourceRect.LowerRightCorner.Y)
		return;

	ox::core::CPosition2d<int> targetPos = pos;
	ox::core::CPosition2d<int> sourcePos = sourceRect.UpperLeftCorner;
	ox::core::CDimension2d<int> sourceSize(sourceRect.getWidth(), sourceRect.getHeight());
	const ox::core::CDimension2d<int> targetSurfaceSize = target->getDimension();

	// clip to cliprect if there is one
	if (clipRect)
	{
		if (targetPos.X < clipRect->UpperLeftCorner.X)
		{
			sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
			if (sourceSize.Width <= 0)
				return;

			sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
			targetPos.X = clipRect->UpperLeftCorner.X;
		}

		if (targetPos.X + sourceSize.Width > clipRect->LowerRightCorner.X)
		{
			sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
			if (sourceSize.Width <= 0)
				return;
		}

		if (targetPos.Y < clipRect->UpperLeftCorner.Y)
		{
			sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
			if (sourceSize.Height <= 0)
				return;

			sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
			targetPos.Y = clipRect->UpperLeftCorner.Y;
		}

		if (targetPos.Y + sourceSize.Height > clipRect->LowerRightCorner.Y)
		{
			sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
			if (sourceSize.Height <= 0)
				return;
		}
	}

	// clip these coordinates to screen

	if (targetPos.X<0)
	{
		sourceSize.Width += targetPos.X;
		if (sourceSize.Width <= 0)
			return;

		sourcePos.X -= targetPos.X;
		targetPos.X = 0;
	}

	if (targetPos.X + sourceSize.Width > targetSurfaceSize.Width)
	{
		sourceSize.Width -= (targetPos.X + sourceSize.Width) - targetSurfaceSize.Width;
		if (sourceSize.Width <= 0)
			return;
	}

	if (targetPos.Y<0)
	{
		sourceSize.Height += targetPos.Y;
		if (sourceSize.Height <= 0)
			return;

		sourcePos.Y -= targetPos.Y;
		targetPos.Y = 0;
	}

	if (targetPos.Y + sourceSize.Height > targetSurfaceSize.Height)
	{
		sourceSize.Height -= (targetPos.Y + sourceSize.Height) - targetSurfaceSize.Height;
		if (sourceSize.Height <= 0)
			return;
	}

	// draw everything

	short* targetData = (short*)target->lock();
	int ltarget = targetPos.Y * targetSurfaceSize.Width + targetPos.X;
	int lsource = sourcePos.Y * Size.Width + sourcePos.X;
	size_t dataRowSizeInBytes = sourceSize.Width * sizeof(short);

	short alphaMask = getAlphaMask();
	short* p;
	short* end;
	short* t;

	int r = getRed(color);
	int g = getGreen(color);
	int b = getBlue(color);

	for (int iy=0; iy<sourceSize.Height; ++iy)
	{
		p = &((short*)Data)[lsource];
		end = p + (dataRowSizeInBytes>>1);
		t = &targetData[ltarget];

		while(p != end)
		{
			if (*p & alphaMask)
				*t = ox::video::RGB16(ox::video::getRed(*p) * (r) >>2, ox::video::getGreen(*p) * (g) >>2, ox::video::getBlue(*p) * (b) >>2);

			++t;
			++p;
		}
		lsource += Size.Width;
		ltarget += targetSurfaceSize.Width;
	}
}



//! draws a line from to
void CImage::drawLine(const ox::core::CPosition2d<int>& from, const ox::core::CPosition2d<int>& to, short color)
{
	if (Format != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}

	int x = from.X;
    int y = from.Y;

	int deltax = to.X - from.X;
	int deltay = to.Y - from.Y;

	int stepx = deltax < 0 ? -1 : 1;
	int stepy = deltay < 0 ? -1 : 1;

	deltax *= stepx; // wie  deltax = abs(deltax);
	deltay *= stepy; // wie  deltay = abs(deltay);

	if (deltax > deltay)
	{
		 int d = deltax >> 1;

		 while (x != to.X)
		 {
			if (x>=0 && y>=0 && x<Size.Width && y<Size.Height)
				setPixelFast(x, y, color);

			d -= deltay;

			if (d < 0)
			{
				y += stepy;
				d += deltax;
			}

			x += stepx;
		 }
	}
	else
	{
		 int d = deltay >> 1;

		 while (y != to.Y)
		 {
			if (x>=0 && y>=0 && x<Size.Width && y<Size.Height)
				setPixelFast(x, y, color);

			d -= deltax;
			if (d < 0)
			{
				x += stepx;
				d += deltay;
			}

			y += stepy;
		 }
	}
}


//! sets a pixel very fast and inline
inline void CImage::setPixelFast(int x, int y, short color)
{
	((short*)Data)[y*Size.Width + x] = color;
}


//! resizes the surface to a new size
void CImage::resizeTo(const ox::core::CDimension2d<int>& size)
{
	if (Format != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}

	// note: this is very very slow. (i didn't want to write a fast version.
	// but hopefully, nobody wants to scale surfaces every frame.

	if (!size.Width || !size.Height)
		return;

	int nDataSize = size.Width * size.Height;
	int nDataSizeInBytes = nDataSize * sizeof(short);
	short* nData = new short[nDataSize];

	float sourceXStep = (float)Size.Width / (float)size.Width;
	float sourceYStep = (float)Size.Height / (float)size.Height;
	float sy;

    for (int x=0; x<size.Width; ++x)
	{
		sy = 0.0f;

		for (int y=0; y<size.Height; ++y)
		{
			nData[(int)(y*size.Width + x)] = ((short*)Data)[(int)(((int)sy)*Size.Width + x*sourceXStep)];
			sy+=sourceYStep;
		}
	}

	delete [] (char*)Data;
    //DataSize = nDataSize;
	//DataSizeInBytes = nDataSizeInBytes;
	Size = size;
	Data = nData;
}



//! copies this surface into another, scaling it to fit it.
void CImage::copyToScaling(CImage* target)
{
	if (Format != ECF_A1R5G5B5 ||
		target->getColorFormat() != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}

	// note: this is very very slow. (i didn't want to write a fast version.
	// but hopefully, nobody wants to scale surfaces every frame.

	ox::core::CDimension2d<int> size = target->getDimension();

	int nDataSize = size.Width * size.Height;
	int nDataSizeInBytes = nDataSize * sizeof(short);

	if (!size.Width || !size.Height)
		return;

	short* nData = (short*)target->lock();

	float sourceXStep = (float)Size.Width / (float)size.Width;
	float sourceYStep = (float)Size.Height / (float)size.Height;
	float sy;

    for (int x=0; x<size.Width; ++x)
	{
		sy = 0.0f;

		for (int y=0; y<size.Height; ++y)
		{
			nData[(int)(y*size.Width + x)] = ((short*)Data)[(int)(((int)sy)*Size.Width + x*sourceXStep)];
			sy+=sourceYStep;
		}
	}

	target->unlock();
}


//! fills the surface with black or white
void CImage::fill(short color)
{
	if (Format != ECF_A1R5G5B5)
	{
		os::Printer::log("CImage is not A1R5G5B5", ox::event::ELL_ERROR);
		return;
	}


	int* p = (int*)Data;
	int* bufferEnd = p + ((Size.Width * Size.Height)>>1);
	int c = ((color & 0x0000ffff)<<16) | (color & 0x0000ffff);
	while(p != bufferEnd)
	{
		*p = c;
		++p;
	}
}



} // end namespace video
} // end namespace daisy
