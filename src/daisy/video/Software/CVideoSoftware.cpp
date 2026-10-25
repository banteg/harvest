// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoSoftware.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/video/IVideoDriver.h"
#include "ox/IOxDevice.h"
#include "ox/video/IImage.h"
#include "ox/video/ITexture.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/io/IFileSystem.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "ox/video/ColorPacking.h"
#include "CVideoSoftware.h"
#include "CSoftwareTexture.h"
#include "daisy/os.h"
#include "ox/video/S3DVertex.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;


//! constructor
CVideoSoftware::CVideoSoftware(const ox::core::CDimension2d<int>& windowSize, bool fullscreen, ox::io::IFileSystem* io, video::IImagePresenter* presenter)
: CVideoNull(io, windowSize), CurrentTriangleRenderer(0), Texture(0),
	 ZBuffer(0), RenderTargetTexture(0), RenderTargetSurface(0)
{

	os::Printer::log("Irrlicht Software Renderer", ox::event::ELL_INFORMATION);

	// create backbuffer

	BackBuffer = new CImage(ECF_A1R5G5B5, windowSize);
	BackBuffer->fill(0);

	// get presenter

	Presenter = presenter;

	// create z buffer

	ZBuffer = video::createZBuffer(BackBuffer->getDimension());

	// create triangle renderers

	TriangleRenderers[ETR_FLAT] = createTriangleRendererFlat(ZBuffer);
	TriangleRenderers[ETR_FLAT_WIRE] = createTriangleRendererFlatWire(ZBuffer);
	TriangleRenderers[ETR_GOURAUD] = createTriangleRendererGouraud(ZBuffer);
	TriangleRenderers[ETR_GOURAUD_WIRE] = createTriangleRendererGouraudWire(ZBuffer);
	TriangleRenderers[ETR_TEXTURE_FLAT] = createTriangleRendererTextureFlat(ZBuffer);
	TriangleRenderers[ETR_TEXTURE_FLAT_WIRE] = createTriangleRendererTextureFlatWire(ZBuffer);
	TriangleRenderers[ETR_TEXTURE_GOURAUD] = createTriangleRendererTextureGouraud(ZBuffer);
	TriangleRenderers[ETR_TEXTURE_GOURAUD_WIRE] = createTriangleRendererTextureGouraudWire(ZBuffer);

	// select render target

	setRenderTarget(BackBuffer);

	// select the right renderer

	selectRightTriangleRenderer();
}



//! destructor
CVideoSoftware::~CVideoSoftware()
{
	// delete Backbuffer
	BackBuffer->drop();

	// delete triangle renderers

	for (int i=0; i<ETR_COUNT; ++i)
		if (TriangleRenderers[i])
			TriangleRenderers[i]->drop();

	// delete zbuffer

	if (ZBuffer)
		ZBuffer->drop();

	// delete current texture

	if (Texture)
		Texture->drop();

	if (RenderTargetTexture)
		RenderTargetTexture->drop();

	if (RenderTargetSurface)
		RenderTargetSurface->drop();
}



//! void selects the right triangle renderer based on the render states.
void CVideoSoftware::selectRightTriangleRenderer()
{
	ETriangleRenderer renderer = ETR_FLAT;

	if (Texture)
	{
		if (!Material.GouraudShading)
			renderer = (!Material.Wireframe) ? ETR_TEXTURE_FLAT : ETR_TEXTURE_FLAT_WIRE;
		else
			renderer = (!Material.Wireframe) ? ETR_TEXTURE_GOURAUD : ETR_TEXTURE_GOURAUD_WIRE;
	}
	else
	{
		if (!Material.GouraudShading)
			renderer = (!Material.Wireframe) ? ETR_FLAT : ETR_FLAT_WIRE;
		else
			renderer = (!Material.Wireframe) ? ETR_GOURAUD : ETR_GOURAUD_WIRE;
	}

	switchToTriangleRenderer(renderer);
}


//! presents the rendered scene on the screen, returns false if failed
bool CVideoSoftware::endScene()
{
	CVideoNull::endScene();

	Presenter->present(BackBuffer);
	return true;
}




//! queries the features of the driver, returns true if feature is available
bool CVideoSoftware::queryFeature(E_VIDEO_DRIVER_FEATURE feature)
{
	switch (feature)
	{
	case EVDF_BILINEAR_FILTER:
		return false;
	case EVDF_RENDER_TO_TARGET:
		return true;
	case EVDF_HARDWARE_TL:
		return false;
	case EVDF_MIP_MAP:
		return false;
	};

	return false;
}



//! sets transformation
void CVideoSoftware::setTransform(E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat)
{
	TransformationMatrix[state] = mat;
}



//! sets the current Texture
void CVideoSoftware::setTexture(ox::video::ITexture* texture)
{
	if (texture && texture->getDriverType() != EDT_SOFTWARE)
	{
		os::Printer::log("Fatal Error: Tried to set a texture not owned by this driver.", ox::event::ELL_ERROR);
		return;
	}

	ox::video::ITexture* oldTexture = texture;

	if (Texture)
		Texture->drop();

	Texture = texture;

	if (Texture)
		Texture->grab();

	selectRightTriangleRenderer();
}




//! sets a material
void CVideoSoftware::setMaterial(const SMaterial& material)
{
	Material = material;
	setTexture(Material.Texture1);
}



//! clears the zbuffer
bool CVideoSoftware::beginScene(bool backBuffer, bool zBuffer, SColor color)
{
	CVideoNull::beginScene(backBuffer, zBuffer, color);

	if (backBuffer)
		BackBuffer->fill(ox::video::toA1R5G5B5(color.color));

	if (ZBuffer)
		ZBuffer->clear();

	return true;
}


//! sets a render target
void CVideoSoftware::setRenderTarget(ox::video::ITexture* texture)
{
	if (texture->getDriverType() != EDT_SOFTWARE)
	{
		os::Printer::log("Fatal Error: Tried to set a texture not owned by this driver.", ox::event::ELL_ERROR);
		return;
	}

	if (RenderTargetTexture)
		RenderTargetTexture->drop();

	RenderTargetTexture = texture;

	if (RenderTargetTexture)
	{
		RenderTargetTexture->grab();
		setRenderTarget(((CSoftwareTexture*)RenderTargetTexture)->getTexture());
	}
	else
		setRenderTarget((video::CImage*)0);
}


//! sets a render target
void CVideoSoftware::setRenderTarget(video::CImage* image)
{
	if (RenderTargetSurface)
		RenderTargetSurface->drop();

	RenderTargetSurface = image;
	RenderTargetSize.Width = 0;
	RenderTargetSize.Height = 0;
	Render2DTranslation.X = 0;
	Render2DTranslation.Y = 0;

	if (RenderTargetSurface)
	{
		RenderTargetSurface->grab();
		RenderTargetSize = RenderTargetSurface->getDimension();
	}

	setViewPort(ox::core::CRect<int>(0,0,RenderTargetSize.Width,RenderTargetSize.Height));

	if (ZBuffer)
		ZBuffer->setSize(RenderTargetSize);
}


//! sets a viewport
void CVideoSoftware::setViewPort(const ox::core::CRect<int>& area)
{
	ViewPort = area;

	//TODO: the clipping is not correct, because the projection is affected.
	// to correct this, ViewPortSize and Render2DTranslation will have to be corrected.
	ox::core::CRect<int> rendert(0,0,RenderTargetSize.Width,RenderTargetSize.Height);
	if ((rendert).LowerRightCorner.X < ViewPort.LowerRightCorner.X) ViewPort.LowerRightCorner.X = (rendert).LowerRightCorner.X; if ((rendert).LowerRightCorner.Y < ViewPort.LowerRightCorner.Y) ViewPort.LowerRightCorner.Y = (rendert).LowerRightCorner.Y; if ((rendert).UpperLeftCorner.X > ViewPort.UpperLeftCorner.X) ViewPort.UpperLeftCorner.X = (rendert).UpperLeftCorner.X; if ((rendert).UpperLeftCorner.Y > ViewPort.UpperLeftCorner.Y) ViewPort.UpperLeftCorner.Y = (rendert).UpperLeftCorner.Y;

	ViewPortSize.Width = ViewPort.getWidth();
	ViewPortSize.Height = ViewPort.getHeight();
	Render2DTranslation.X = (ViewPortSize.Width / 2) + ViewPort.UpperLeftCorner.X;
	Render2DTranslation.Y = ViewPort.UpperLeftCorner.Y + ViewPortSize.Height - (ViewPortSize.Height / 2);// + ViewPort.UpperLeftCorner.Y;

	if (CurrentTriangleRenderer)
		CurrentTriangleRenderer->setRenderTarget(RenderTargetSurface, ViewPort);
}



//! draws an indexed triangle list
void CVideoSoftware::drawIndexedTriangleList(const S3DVertex* vertices, int vertexCount, const unsigned short* indexList, int triangleCount)
{
	if (!RenderTargetSurface || !ZBuffer)
		return;

	if (!checkPrimitiveCount(vertexCount))
		return;

	CVideoNull::drawIndexedTriangleList(vertices, vertexCount, indexList, triangleCount);

	if ((int)TransformedPoints.size() < vertexCount)
		TransformedPoints.resize(vertexCount);

	const S3DVertex* currentVertex = vertices;
	S2DVertex* tp = &TransformedPoints[0];

	ox::core::CDimension2d<int> textureSize(0,0);
	float zDiv;

	if (Texture)
		textureSize = ((CSoftwareTexture*)Texture)->getTexture()->getDimension();

	float transformedPos[4]; // transform all points in the list

	ox::core::CMatrix4 matrix(TransformationMatrix[ETS_PROJECTION]);
	matrix *= TransformationMatrix[ETS_VIEW];
	matrix *= TransformationMatrix[ETS_WORLD];

	int ViewTransformWidth = (ViewPortSize.Width>>1);
	int ViewTransformHeight = (ViewPortSize.Height>>1);

	for (int i=0; i<vertexCount; ++i)
	{
		transformedPos[0] = currentVertex->Pos.X;
		transformedPos[1] = currentVertex->Pos.Y;
		transformedPos[2] = currentVertex->Pos.Z;
		transformedPos[3] = 1.0f;

		matrix.multiplyWith1x4Matrix(transformedPos);
		zDiv = transformedPos[3] == 0.0f ? 1.0f : (1.0f / transformedPos[3]);

		tp->Pos.X = (int)(ViewTransformWidth * (transformedPos[0] * zDiv) + (Render2DTranslation.X));
		tp->Pos.Y = (Render2DTranslation.Y - (int)(ViewTransformHeight * (transformedPos[1] * zDiv)));
		tp->Color = ox::video::toA1R5G5B5(currentVertex->Color.color);
		tp->ZValue = (TZBufferType)(32767.0f * zDiv);

		tp->TCoords.X = (int)(currentVertex->TCoords.X * textureSize.Width);
		tp->TCoords.X <<= 8;
		tp->TCoords.Y = (int)(currentVertex->TCoords.Y * textureSize.Height);
		tp->TCoords.Y <<= 8;

		++currentVertex;
		++tp;
	}


	// draw all transformed points from the index list
	CurrentTriangleRenderer->drawIndexedTriangleList(&TransformedPoints[0],
		vertexCount, indexList, triangleCount);
}


//! draws an indexed triangle list
void CVideoSoftware::drawIndexedTriangleList(const S3DVertex2TCoords* vertices, int vertexCount, const unsigned short* indexList, int triangleCount)
{
	if (!RenderTargetSurface || !ZBuffer)
		return;

	if (!checkPrimitiveCount(vertexCount))
		return;

	CVideoNull::drawIndexedTriangleList(vertices, vertexCount, indexList, triangleCount);

	if ((int)TransformedPoints.size() < vertexCount)
		TransformedPoints.resize(vertexCount);

	const S3DVertex2TCoords* currentVertex = vertices;
	S2DVertex* tp = &TransformedPoints[0];

	ox::core::CDimension2d<int> textureSize(0,0);
	float zDiv;

	if (Texture)
		textureSize = ((CSoftwareTexture*)Texture)->getTexture()->getDimension();

	float transformedPos[4]; // transform all points in the list

	ox::core::CMatrix4 matrix(TransformationMatrix[ETS_PROJECTION]);
	matrix *= TransformationMatrix[ETS_VIEW];
	matrix *= TransformationMatrix[ETS_WORLD];

	int ViewTransformWidth = (ViewPortSize.Width>>1);
	int ViewTransformHeight = (ViewPortSize.Height>>1);

	for (int i=0; i<vertexCount; ++i)
	{
		transformedPos[0] = currentVertex->Pos.X;
		transformedPos[1] = currentVertex->Pos.Y;
		transformedPos[2] = currentVertex->Pos.Z;
		transformedPos[3] = 1.0f;

		matrix.multiplyWith1x4Matrix(transformedPos);
		zDiv = transformedPos[3] == 0.0f ? 1.0f : (1.0f / transformedPos[3]);

		tp->Pos.X = (int)(ViewTransformWidth * (transformedPos[0] * zDiv) + (Render2DTranslation.X));
		tp->Pos.Y = (Render2DTranslation.Y - (int)(ViewTransformHeight * (transformedPos[1] * zDiv)));
		tp->Color = ox::video::toA1R5G5B5(currentVertex->Color.color);
		tp->ZValue = (TZBufferType)(32767.0f * zDiv);

		tp->TCoords.X = (int)(currentVertex->TCoords.X * textureSize.Width);
		tp->TCoords.X <<= 8;
		tp->TCoords.Y = (int)(currentVertex->TCoords.Y * textureSize.Height);
		tp->TCoords.Y <<= 8;

		++currentVertex;
		++tp;
	}


	// draw all transformed points from the index list
	CurrentTriangleRenderer->drawIndexedTriangleList(&TransformedPoints[0],
		vertexCount, indexList, triangleCount);
}



//! draws an indexed triangle fan
void CVideoSoftware::drawIndexedTriangleFan(const S3DVertex* vertices, int vertexCount, const unsigned short* indexList, int triangleCount)
{
	// TODO: don't convert fan to list

	ox::TArray<unsigned short> newBuffer;

	for( int t=0; t<triangleCount; t++ )
	{
		newBuffer.push_back(indexList[0]);
		newBuffer.push_back(indexList[t+1]);
		newBuffer.push_back(indexList[t+2]);
	}

	drawIndexedTriangleList( vertices, vertexCount, &newBuffer[0], triangleCount );
}


//! draws an indexed triangle fan
void CVideoSoftware::drawIndexedTriangleFan(const S3DVertex2TCoords* vertices, int vertexCount, const unsigned short* indexList, int triangleCount)
{
	// TODO: don't convert fan to list

	ox::TArray<unsigned short> newBuffer;

	for( int t=0; t<triangleCount; t++ )
	{
		newBuffer.push_back(indexList[0]);
		newBuffer.push_back(indexList[t+1]);
		newBuffer.push_back(indexList[t+2]);
	}

	drawIndexedTriangleList( vertices, vertexCount, &newBuffer[0], triangleCount );
}



//! clips a triangle agains the viewing frustrum
void CVideoSoftware::clipTriangle(float* transformedPos)
{

}


//! creates the clipping planes from the matrix
void CVideoSoftware::createPlanes(ox::core::CMatrix4& mat)
{
	Frustrum = ox::scene::SViewFrustrum(mat);
}


//! draws an 2d image
void CVideoSoftware::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos)
{
	if (texture)
	{
		if (texture->getDriverType() != EDT_SOFTWARE)
		{
			os::Printer::log("Fatal Error: Tried to copy from a surface not owned by this driver.", ox::event::ELL_ERROR);
			return;
		}

		((CSoftwareTexture*)texture)->getImage()->copyTo(BackBuffer, destPos);
	}
}

/*
void CVideoSoftware::drawIndexedTriangleList(const S3DVertex2TCoords* vertices, int vertexCount, const unsigned short* indexList, int triangleCount)
{
	if (!RenderTargetSurface || !ZBuffer)
		return;

	CVideoNull::drawIndexedTriangleList(vertices, vertexCount, indexList, triangleCount);

	if ((int)TransformedPoints.size() < triangleCount*3)
		TransformedPoints.resize(triangleCount*3);

	int tranformed = 0;
	ox::core::CDimension2d<int> textureSize(0,0);
	float zDiv;

	if (Texture)
		textureSize = ((CSoftwareTexture*)Texture)->getTexture()->getDimension();

	ox::core::CMatrix4 matrix(TransformationMatrix[TS_PROJECTION]);
	matrix *= TransformationMatrix[TS_VIEW];
	matrix *= TransformationMatrix[TS_WORLD];

	createPlanes(matrix);

	int ViewTransformWidth = (ViewPortSize.Width>>1);
	int ViewTransformHeight = (ViewPortSize.Height>>1);

	const S3DVertex2TCoords* currentVertex;

	ox::TArray<unsigned short> newIndices;

	float transformedPos[12]; // transform all points in the list

	for (int i=0; i<triangleCount*3; i+=3)
	{
		int t;
		bool clipped = false;

		for (t=0; t<3; ++t)
		{
			transformedPos[(t*4)+0] = vertices[indexList[i+t]].Pos.X;
			transformedPos[(t*4)+1] = vertices[indexList[i+t]].Pos.Y;
			transformedPos[(t*4)+2] = vertices[indexList[i+t]].Pos.Z;
			transformedPos[(t*4)+3] = 1.0f;

			matrix.multiplyWith1x4Matrix(&transformedPos[t*4]);

			if (transformedPos[(t*4)+3] < 0)
			{
				clipped = true;
				break;
			}
		}

		if (!clipped)
		{
			for (t=0; t<3; ++t)
			{
				S2DVertex* tp = &TransformedPoints[tranformed];
				currentVertex = &vertices[indexList[i+t]];

				zDiv = transformedPos[(t*4)+3] == 0.0f ? 1.0f : (1.0f / transformedPos[(t*4)+3]);

				tp->Pos.X = (int)(ViewTransformWidth * (transformedPos[(t*4)+0] * zDiv) + (Render2DTranslation.X));
				tp->Pos.Y = (Render2DTranslation.Y - (int)(ViewTransformHeight * (transformedPos[(t*4)+1] * zDiv)));
				tp->Color = ox::video::toA1R5G5B5(currentVertex->Color.color);
				tp->ZValue = (TZBufferType)(32767.0f * zDiv);

				tp->TCoords.X = (int)(currentVertex->TCoords.X * textureSize.Width);
				tp->TCoords.X <<= 8;
				tp->TCoords.Y = (int)(currentVertex->TCoords.Y * textureSize.Height);
				tp->TCoords.Y <<= 8;

				++tranformed;
			}

			newIndices.push_back(tranformed-3);
			newIndices.push_back(tranformed-2);
			newIndices.push_back(tranformed-1);
		}

	}

	// draw all transformed points from the index list
	CurrentTriangleRenderer->drawIndexedTriangleList(&TransformedPoints[0],
		vertexCount, &newIndices[0], newIndices.size() / 3);
}*/



//! Draws a 2d line.
void CVideoSoftware::draw2DLine(const ox::core::CPosition2d<int>& start,
								const ox::core::CPosition2d<int>& end,
								SColor color)
{
	((CImage*)BackBuffer)->drawLine(start, end, ox::video::toA1R5G5B5(color.color));
}


//! draws an 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
void CVideoSoftware::draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos,
								 const ox::core::CRect<int>& sourceRect,
								 const ox::core::CRect<int>* clipRect, SColor color,
								 bool useAlphaChannelOfTexture)
{
	if (texture)
	{
		if (texture->getDriverType() != EDT_SOFTWARE)
		{
			os::Printer::log("Fatal Error: Tried to copy from a surface not owned by this driver.", ox::event::ELL_ERROR);
			return;
		}

		if (useAlphaChannelOfTexture)
			((CSoftwareTexture*)texture)->getImage()->copyToWithAlpha(BackBuffer, destPos, sourceRect, ox::video::toA1R5G5B5(color.color), clipRect);
		else
			((CSoftwareTexture*)texture)->getImage()->copyTo(BackBuffer, destPos, sourceRect, clipRect);
	}
}




//! draw an 2d rectangle
void CVideoSoftware::draw2DRectangle(SColor color, const ox::core::CRect<int>& pos, const ox::core::CRect<int>* clip)
{
	if (clip)
	{
		ox::core::CRect<int> p(pos);
		if ((*clip).LowerRightCorner.X < p.LowerRightCorner.X) p.LowerRightCorner.X = (*clip).LowerRightCorner.X; if ((*clip).LowerRightCorner.Y < p.LowerRightCorner.Y) p.LowerRightCorner.Y = (*clip).LowerRightCorner.Y; if ((*clip).UpperLeftCorner.X > p.UpperLeftCorner.X) p.UpperLeftCorner.X = (*clip).UpperLeftCorner.X; if ((*clip).UpperLeftCorner.Y > p.UpperLeftCorner.Y) p.UpperLeftCorner.Y = (*clip).UpperLeftCorner.Y;
		BackBuffer->drawRectangle(p, color);
	}
	else
		BackBuffer->drawRectangle(pos, color);
}



//! \return Returns the name of the video driver. Example: In case of the DirectX8
//! driver, it would return "Direct3D8.1".
const wchar_t* CVideoSoftware::getName()
{
	return L"IrrlichtSoftwareDevice";
}


//! switches to a triangle renderer
void CVideoSoftware::switchToTriangleRenderer(ETriangleRenderer renderer)
{
	ox::video::IImage* s = 0;
	if (Texture)
		s = ((CSoftwareTexture*)Texture)->getTexture();

	CurrentTriangleRenderer = TriangleRenderers[renderer];
	CurrentTriangleRenderer->setBackfaceCulling(Material.BackfaceCulling == true);
	CurrentTriangleRenderer->setTexture(s);
	CurrentTriangleRenderer->setRenderTarget(RenderTargetSurface, ViewPort);
}

//! Returns type of video driver
int CVideoSoftware::getDriverType()
{
	return EDT_SOFTWARE;
}

//! Returns the transformation set by setTransform
ox::core::CMatrix4 CVideoSoftware::getTransform(E_TRANSFORMATION_STATE state)
{
	return TransformationMatrix[state];
}


//! creates a video driver
IVideoDriver* createSoftwareDriver(const ox::core::CDimension2d<int>& windowSize, bool fullscreen, ox::io::IFileSystem* io, video::IImagePresenter* presenter)
{
	return new CVideoSoftware(windowSize, fullscreen, io, presenter);
}


} // end namespace video
} // end namespace daisy
