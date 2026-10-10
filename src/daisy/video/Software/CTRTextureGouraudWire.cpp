// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTRTextureGouraudWire.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CTRTextureGouraud.h"
#include "ox/core/CRect.h"
#include "ox/video/SColor.h"
#include "ox/video/ColorPacking.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;

class CTRTextureGouraudWire : public CTRTextureGouraud
{
public:

	CTRTextureGouraudWire(IZBuffer* zbuffer)
		: CTRTextureGouraud(zbuffer)
	{
	}

	//! draws an indexed triangle list
	virtual void drawIndexedTriangleList(S2DVertex* vertices, int vertexCount, const unsigned short* indexList, int triangleCount)
	{
		const S2DVertex *v1, *v2, *v3;

		short color;
		float tmpDiv; // temporary division factor
		float longest; // saves the longest span
		int height; // saves height of triangle
		short* targetSurface; // target pointer where to plot pixels
		int spanEnd; // saves end of spans
		float leftdeltaxf; // amount of pixels to increase on left side of triangle
		float rightdeltaxf; // amount of pixels to increase on right side of triangle
		int leftx, rightx; // position where we are 
		float leftxf, rightxf; // same as above, but as float values
		int span; // current span
		int leftR, leftG, leftB, rightR, rightG, rightB; // color values
		int leftStepR, leftStepG, leftStepB,
			rightStepR, rightStepG, rightStepB; // color steps
		int leftTx, rightTx, leftTy, rightTy; // texture interpolating values
		int leftTxStep, rightTxStep, leftTyStep, rightTyStep; // texture interpolating values
		ox::core::CRect<int> TriangleRect;

		int leftZValue, rightZValue;
		int leftZStep, rightZStep;
		TZBufferType* zTarget;//, *spanZTarget; // target of ZBuffer;

		lockedSurface = (short*)RenderTarget->lock();
		lockedZBuffer = ZBuffer->lock();
		lockedTexture = (short*)Texture->lock();

		for (int i=0; i<triangleCount; ++i)
		{
			v1 = &vertices[*indexList];
			++indexList;
			v2 = &vertices[*indexList];
			++indexList;
			v3 = &vertices[*indexList];
			++indexList;

			// back face culling

			if (BackFaceCullingEnabled)
			{
				int z = ((v3->Pos.X - v1->Pos.X) * (v3->Pos.Y - v2->Pos.Y)) -
					((v3->Pos.Y - v1->Pos.Y) * (v3->Pos.X - v2->Pos.X));

				if (z < 0)
					continue;
			}

			//near plane clipping

			if (v1->ZValue<0 && v2->ZValue<0 && v3->ZValue<0)
				continue;

			// sort for width for inscreen clipping

			if (v1->Pos.X > v2->Pos.X)	swapVertices(&v1, &v2);
			if (v1->Pos.X > v3->Pos.X)	swapVertices(&v1, &v3);
			if (v2->Pos.X > v3->Pos.X)	swapVertices(&v2, &v3);

			if ((v1->Pos.X - v3->Pos.X) == 0)
				continue;

			TriangleRect.UpperLeftCorner.X = v1->Pos.X;
			TriangleRect.LowerRightCorner.X = v3->Pos.X;

			// sort for height for faster drawing.

			if (v1->Pos.Y > v2->Pos.Y)	swapVertices(&v1, &v2);
			if (v1->Pos.Y > v3->Pos.Y)	swapVertices(&v1, &v3);
			if (v2->Pos.Y > v3->Pos.Y)	swapVertices(&v2, &v3);

			TriangleRect.UpperLeftCorner.Y = v1->Pos.Y;
			TriangleRect.LowerRightCorner.Y = v3->Pos.Y;

			if (!TriangleRect.isRectCollided(ViewPortRect))
				continue;


			// höhe des dreiecks berechnen
			height = v3->Pos.Y - v1->Pos.Y;
			if (!height)
				continue;

			// calculate longest span

			longest = (v2->Pos.Y - v1->Pos.Y) / (float)height * (v3->Pos.X - v1->Pos.X) + (v1->Pos.X - v2->Pos.X);

			spanEnd = v2->Pos.Y;
			span = v1->Pos.Y;
			leftxf = (float)v1->Pos.X;
			rightxf = (float)v1->Pos.X;

			leftZValue = v1->ZValue;
			rightZValue = v1->ZValue;

			leftR = rightR = ox::video::getRed(v1->Color)<<8;
			leftG = rightG = ox::video::getGreen(v1->Color)<<8;
			leftB = rightB = ox::video::getBlue(v1->Color)<<8;
			leftTx = rightTx = v1->TCoords.X;
			leftTy = rightTy = v1->TCoords.Y;

			targetSurface = lockedSurface + span * SurfaceWidth;
			zTarget = lockedZBuffer + span * SurfaceWidth;

			if (longest < 0.0f)
			{
				tmpDiv = 1.0f / (float)(v2->Pos.Y - v1->Pos.Y);
				rightdeltaxf = (v2->Pos.X - v1->Pos.X) * tmpDiv;
				rightZStep = (int)((v2->ZValue - v1->ZValue) * tmpDiv);
				rightStepR = (int)(((ox::video::getRed(v2->Color)<<8) - rightR) * tmpDiv);
				rightStepG = (int)(((ox::video::getGreen(v2->Color)<<8) - rightG) * tmpDiv);
				rightStepB = (int)(((ox::video::getBlue(v2->Color)<<8) - rightB) * tmpDiv);
				rightTxStep = (int)((v2->TCoords.X - rightTx) * tmpDiv);
				rightTyStep = (int)((v2->TCoords.Y - rightTy) * tmpDiv);

				tmpDiv = 1.0f / (float)height;
				leftdeltaxf = (v3->Pos.X - v1->Pos.X) * tmpDiv;
				leftZStep = (int)((v3->ZValue - v1->ZValue) * tmpDiv);
				leftStepR = (int)(((ox::video::getRed(v3->Color)<<8) - leftR) * tmpDiv);
				leftStepG = (int)(((ox::video::getGreen(v3->Color)<<8) - leftG) * tmpDiv);
				leftStepB = (int)(((ox::video::getBlue(v3->Color)<<8) - leftB) * tmpDiv);
				leftTxStep = (int)((v3->TCoords.X - leftTx) * tmpDiv);
				leftTyStep = (int)((v3->TCoords.Y - leftTy) * tmpDiv);
			}
			else
			{
				tmpDiv = 1.0f / (float)height;
				rightdeltaxf = (v3->Pos.X - v1->Pos.X) * tmpDiv;
				rightZStep = (int)((v3->ZValue - v1->ZValue) * tmpDiv);
				rightStepR = (int)(((ox::video::getRed(v3->Color)<<8) - rightR) * tmpDiv);
				rightStepG = (int)(((ox::video::getGreen(v3->Color)<<8) - rightG) * tmpDiv);
				rightStepB = (int)(((ox::video::getBlue(v3->Color)<<8) - rightB) * tmpDiv);
				rightTxStep = (int)((v3->TCoords.X - rightTx) * tmpDiv);
				rightTyStep = (int)((v3->TCoords.Y - rightTy) * tmpDiv);

				tmpDiv = 1.0f / (float)(v2->Pos.Y - v1->Pos.Y);
				leftdeltaxf = (v2->Pos.X - v1->Pos.X) * tmpDiv;
				leftZStep = (int)((v2->ZValue - v1->ZValue) * tmpDiv);
				leftStepR = (int)(((ox::video::getRed(v2->Color)<<8) - leftR) * tmpDiv);
				leftStepG = (int)(((ox::video::getGreen(v2->Color)<<8) - leftG) * tmpDiv);
				leftStepB = (int)(((ox::video::getBlue(v2->Color)<<8) - leftB) * tmpDiv);
				leftTxStep = (int)((v2->TCoords.X - leftTx) * tmpDiv);
				leftTyStep = (int)((v2->TCoords.Y - leftTy) * tmpDiv);
			}


			// do it twice, once for the first half of the triangle,
			// end then for the second half.

			for (int triangleHalf=0; triangleHalf<2; ++triangleHalf)
			{
				if (spanEnd > ViewPortRect.LowerRightCorner.Y)
					spanEnd = ViewPortRect.LowerRightCorner.Y;

				// if the span <0, than we can skip these spans, 
				// and proceed to the next spans which are really on the screen.
				if (span < ViewPortRect.UpperLeftCorner.Y)
				{
					// we'll use leftx as temp variable
					if (spanEnd < ViewPortRect.UpperLeftCorner.Y)
					{
						leftx = spanEnd - span;
						span = spanEnd;
					}
					else
					{
						leftx = ViewPortRect.UpperLeftCorner.Y - span; 
						span = ViewPortRect.UpperLeftCorner.Y;
					}

					leftxf += leftdeltaxf*leftx;
					rightxf += rightdeltaxf*leftx;
					targetSurface += SurfaceWidth*leftx;
					zTarget += SurfaceWidth*leftx;
					leftZValue += leftZStep*leftx;
					rightZValue += rightZStep*leftx;

					leftR += leftStepR*leftx;
					leftG += leftStepG*leftx;
					leftB += leftStepB*leftx;
					rightR += rightStepR*leftx;
					rightG += rightStepG*leftx;
					rightB += rightStepB*leftx;

					leftTx += leftTxStep*leftx;
					leftTy += leftTyStep*leftx;
					rightTx += rightTxStep*leftx;
					rightTy += rightTyStep*leftx;
				}


				// the main loop. Go through every span and draw it.

				while (span < spanEnd)
				{
					leftx = (int)(leftxf);
					rightx = (int)(rightxf + 0.5f);

					// perform some clipping

					if (leftx>=ViewPortRect.UpperLeftCorner.X &&
						leftx<=ViewPortRect.LowerRightCorner.X)
					{
						if (leftZValue > *(zTarget + leftx))
						{
							*(zTarget + leftx) = leftZValue;
							color = lockedTexture[((leftTy>>8)&textureYMask) * lockedTextureWidth + ((leftTx>>8)&textureXMask)];
							*(targetSurface + leftx) = ox::video::RGB16(ox::video::getRed(color) * (leftR>>8) >>2, ox::video::getGreen(color) * (leftG>>8) >>2, ox::video::getBlue(color) * (leftR>>8) >>2);
						}
					}


					if (rightx>=ViewPortRect.UpperLeftCorner.X &&
						rightx<=ViewPortRect.LowerRightCorner.X)
					{
						if (rightZValue > *(zTarget + rightx))
						{
							*(zTarget + rightx) = rightZValue;
							color = lockedTexture[((rightTy>>8)&textureYMask) * lockedTextureWidth + ((rightTx>>8)&textureXMask)];
							*(targetSurface + rightx) = ox::video::RGB16(ox::video::getRed(color) * (rightR>>8) >>2, ox::video::getGreen(color) * (rightG>>8) >>2, ox::video::getBlue(color) * (rightR>>8) >>2);
						}

					}

					leftxf += leftdeltaxf;
					rightxf += rightdeltaxf;
					++span;
					targetSurface += SurfaceWidth;
					zTarget += SurfaceWidth;
					leftZValue += leftZStep;
					rightZValue += rightZStep;

					leftR += leftStepR;
					leftG += leftStepG;
					leftB += leftStepB;
					rightR += rightStepR;
					rightG += rightStepG;
					rightB += rightStepB;

					leftTx += leftTxStep;
					leftTy += leftTyStep;
					rightTx += rightTxStep;
					rightTy += rightTyStep;
				}

				if (triangleHalf>0) // break, we've gout only two halves
					break;


				// setup variables for second half of the triangle.

				if (longest < 0.0f)
				{
					tmpDiv = 1.0f / (v3->Pos.Y - v2->Pos.Y);

					rightdeltaxf = (v3->Pos.X - v2->Pos.X) * tmpDiv;
					rightxf = (float)v2->Pos.X;

					rightZValue = v2->ZValue;
					rightZStep = (int)((v3->ZValue - v2->ZValue) * tmpDiv);

					rightR = ox::video::getRed(v2->Color)<<8;
					rightG = ox::video::getGreen(v2->Color)<<8;
					rightB = ox::video::getBlue(v2->Color)<<8;
					rightStepR = (int)(((ox::video::getRed(v3->Color)<<8) - rightR) * tmpDiv);
					rightStepG = (int)(((ox::video::getGreen(v3->Color)<<8) - rightG) * tmpDiv);
					rightStepB = (int)(((ox::video::getBlue(v3->Color)<<8) - rightB) * tmpDiv);

					rightTx = v2->TCoords.X;
					rightTy = v2->TCoords.Y;
					rightTxStep = (int)((v3->TCoords.X - rightTx) * tmpDiv);
					rightTyStep = (int)((v3->TCoords.Y - rightTy) * tmpDiv);
				}
				else
				{
					tmpDiv = 1.0f / (v3->Pos.Y - v2->Pos.Y);

					leftdeltaxf = (v3->Pos.X - v2->Pos.X) * tmpDiv;
					leftxf = (float)v2->Pos.X;

					leftZValue = v2->ZValue;
					leftZStep = (int)((v3->ZValue - v2->ZValue) * tmpDiv);

					leftR = ox::video::getRed(v2->Color)<<8;
					leftG = ox::video::getGreen(v2->Color)<<8;
					leftB = ox::video::getBlue(v2->Color)<<8;
					leftStepR = (int)(((ox::video::getRed(v3->Color)<<8) - leftR) * tmpDiv);
					leftStepG = (int)(((ox::video::getGreen(v3->Color)<<8) - leftG) * tmpDiv);
					leftStepB = (int)(((ox::video::getBlue(v3->Color)<<8) - leftB) * tmpDiv);

					leftTx = v2->TCoords.X;
					leftTy = v2->TCoords.Y;
					leftTxStep = (int)((v3->TCoords.X - leftTx) * tmpDiv);
					leftTyStep = (int)((v3->TCoords.Y - leftTy) * tmpDiv);
				}


				spanEnd = v3->Pos.Y;
			}

		}

		RenderTarget->unlock();
		ZBuffer->unlock();
		Texture->unlock();

	}

};


//! creates a flat triangle renderer
IK3DTriangleRenderer* createTriangleRendererTextureGouraudWire(IZBuffer* zbuffer)
{
	return new CTRTextureGouraudWire(zbuffer);
}

} // end namespace video
} // end namespace daisy
