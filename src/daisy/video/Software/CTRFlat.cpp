// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTRFlat.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
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

class CTRFlat : public CTRTextureGouraud
{
public:

	CTRFlat(IZBuffer* zbuffer)
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
		short *hSpanBegin, *hSpanEnd; // pointer used when plotting pixels
		ox::core::CRect<int> TriangleRect;

		int leftZValue, rightZValue;
		int leftZStep, rightZStep;
		int spanZValue, spanZStep; // ZValues when drawing a span
		TZBufferType* zTarget, *spanZTarget; // target of ZBuffer;

		lockedSurface = (short*)RenderTarget->lock();
		lockedZBuffer = ZBuffer->lock();
		
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

			color = v1->Color;

			targetSurface = lockedSurface + span * SurfaceWidth;
			zTarget = lockedZBuffer + span * SurfaceWidth;

			if (longest < 0.0f)
			{
				tmpDiv = 1.0f / (float)(v2->Pos.Y - v1->Pos.Y);
				rightdeltaxf = (v2->Pos.X - v1->Pos.X) * tmpDiv;
				rightZStep = (int)((v2->ZValue - v1->ZValue) * tmpDiv);

				tmpDiv = 1.0f / (float)height;
				leftdeltaxf = (v3->Pos.X - v1->Pos.X) * tmpDiv;
				leftZStep = (int)((v3->ZValue - v1->ZValue) * tmpDiv);
			}
			else
			{
				tmpDiv = 1.0f / (float)height;
				rightdeltaxf = (v3->Pos.X - v1->Pos.X) * tmpDiv;
				rightZStep = (int)((v3->ZValue - v1->ZValue) * tmpDiv);

				tmpDiv = 1.0f / (float)(v2->Pos.Y - v1->Pos.Y);
				leftdeltaxf = (v2->Pos.X - v1->Pos.X) * tmpDiv;
				leftZStep = (int)((v2->ZValue - v1->ZValue) * tmpDiv);
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
				}


				// the main loop. Go through every span and draw it.

				while (span < spanEnd)
				{
					leftx = (int)(leftxf);
					rightx = (int)(rightxf + 0.5f);

					// perform some clipping

					// TODO: clipping is not correct when leftx is clipped.

					if (leftx<ViewPortRect.UpperLeftCorner.X)
						leftx = ViewPortRect.UpperLeftCorner.X;
					else
						if (leftx>ViewPortRect.LowerRightCorner.X)
							leftx = ViewPortRect.LowerRightCorner.X;

					if (rightx<ViewPortRect.UpperLeftCorner.X)
						rightx = ViewPortRect.UpperLeftCorner.X;
					else
						if (rightx>ViewPortRect.LowerRightCorner.X)
							rightx = ViewPortRect.LowerRightCorner.X;

					// draw the span

					if (rightx - leftx != 0)
					{
						tmpDiv = 1.0f / (rightx - leftx);
						spanZValue = leftZValue;
						spanZStep = (int)((rightZValue - leftZValue) * tmpDiv);

						hSpanBegin = targetSurface + leftx;
						spanZTarget = zTarget + leftx;
						hSpanEnd = targetSurface + rightx;

						while (hSpanBegin < hSpanEnd)
						{
							if (spanZValue > *spanZTarget)
							{
								*spanZTarget = spanZValue;
								*hSpanBegin = color;
							}

							spanZValue += spanZStep;
							++hSpanBegin;
							++spanZTarget;
						}
					}

					leftxf += leftdeltaxf;
					rightxf += rightdeltaxf;
					++span;
					targetSurface += SurfaceWidth;
					zTarget += SurfaceWidth;
					leftZValue += leftZStep;
					rightZValue += rightZStep;
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
				}
				else
				{
					tmpDiv = 1.0f / (v3->Pos.Y - v2->Pos.Y);

					leftdeltaxf = (v3->Pos.X - v2->Pos.X) * tmpDiv;
					leftxf = (float)v2->Pos.X;

					leftZValue = v2->ZValue;
					leftZStep = (int)((v3->ZValue - v2->ZValue) * tmpDiv);
				}


				spanEnd = v3->Pos.Y;
			}

		}

		RenderTarget->unlock();
		ZBuffer->unlock();
	}
};


//! creates a flat triangle renderer
IK3DTriangleRenderer* createTriangleRendererFlat(IZBuffer* zbuffer)
{
	return new CTRFlat(zbuffer);
}

} // end namespace video
} // end namespace daisy
