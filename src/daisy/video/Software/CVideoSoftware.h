// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoSoftware.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_K_VIDEO_SOFTWARE_H_INCLUDED__
#define __C_K_VIDEO_SOFTWARE_H_INCLUDED__

#include "IK3DTriangleRenderer.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CVector3d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/IImage.h"
#include "ox/video/ITexture.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/io/IFileSystem.h"
#include "ox/TArray.h"
#include "daisy/video/IImagePresenter.h"
#include "daisy/video/Software/IZBuffer.h"
#include "daisy/video/Null/CVideoNull.h"
#include "ox/scene/SViewFrustrum.h"
#include "daisy/video/Null/CImage.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;
	class CVideoSoftware : public CVideoNull
	{
	public:

		//! constructor
		CVideoSoftware(const ox::core::CDimension2d<int>& windowSize, bool fullscreen, ox::io::IFileSystem* io, video::IImagePresenter* presenter);

		//! destructor
		virtual ~CVideoSoftware();
		virtual void useMaterialShaderFor2D(bool enabled);

		//! presents the rendered scene on the screen, returns false if failed
		virtual bool endScene();

		//! queries the features of the driver, returns true if feature is available
		virtual bool queryFeature(E_VIDEO_DRIVER_FEATURE feature);

		//! sets transformation
		virtual void setTransform(E_TRANSFORMATION_STATE state, const ox::core::CMatrix4& mat);

		//! sets a material
		virtual void setMaterial(const SMaterial& material);

		//! sets a render target
		virtual bool setRenderTarget(ox::video::ITexture* texture);

		//! sets a viewport
		virtual void setViewPort(const ox::core::CRect<int>& area);

		//! clears the zbuffer
		virtual bool beginScene(bool backBuffer, bool zBuffer, SColor color);

		//! draws an indexed triangle list
		virtual void drawIndexedTriangleList(const S3DVertex* vertices, int vertexCount, const unsigned short* indexList, int triangleCount);

		//! draws an indexed triangle list
		virtual void drawIndexedTriangleList(const S3DVertex2TCoords* vertices, int vertexCount, const unsigned short* indexList, int triangleCount);

		//! Draws an indexed triangle fan.
		virtual void drawIndexedTriangleFan(const S3DVertex* vertices,
			int vertexCount, const unsigned short* indexList, int triangleCount);

		//! Draws an indexed triangle fan.
		virtual void drawIndexedTriangleFan(const S3DVertex2TCoords* vertices,
			int vertexCount, const unsigned short* indexList, int triangleCount);

		//! draws an 2d image
		virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos);

		//! draws an 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
		virtual void draw2DImage(ox::video::ITexture* texture, const ox::core::CPosition2d<int>& destPos,
			const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect = 0,
			SColor color=SColor(255,255,255,255), bool useAlphaChannelOfTexture=false);

		//! draw an 2d rectangle
		virtual void draw2DRectangle(SColor color, const ox::core::CRect<int>& pos,
			const ox::core::CRect<int>* clip = 0);

		//! Draws a 2d line.
		virtual void draw2DLine(const ox::core::CPosition2d<int>& start,
								const ox::core::CPosition2d<int>& end,
								SColor color=SColor(255,255,255,255));

		//! \return Returns the name of the video driver. Example: In case of the DirectX8
		//! driver, it would return "Direct3D8.1".
		virtual const wchar_t* getName();

		//! Returns type of video driver
		virtual int getDriverType();
		virtual bool isFullscreen();

		//! Returns the transformation set by setTransform
		virtual ox::core::CMatrix4 getTransform(E_TRANSFORMATION_STATE state);

	protected:

		struct splane
		{
			ox::core::CVector3d<float> Normal;
			float Dist;
		};

		//! sets a render target
		void setRenderTarget(video::CImage* image);

		//! sets the current Texture
		void setTexture(ox::video::ITexture* texture);

		video::CImage* BackBuffer;
		video::IImagePresenter* Presenter;

		//! switches to a triangle renderer
		void switchToTriangleRenderer(ETriangleRenderer renderer);

		//! void selects the right triangle renderer based on the render states.
		void selectRightTriangleRenderer();

		//! clips a triangle agains the viewing frustrum
		void clipTriangle(float* transformedPos);

		//! creates the clipping planes from the view matrix
		void createPlanes(ox::core::CMatrix4& mat);

		ox::TArray<S2DVertex> TransformedPoints;

		ox::video::ITexture* RenderTargetTexture;
		ox::video::IImage* RenderTargetSurface;
		ox::core::CPosition2d<int> Render2DTranslation;
		ox::core::CDimension2d<int> RenderTargetSize;
		ox::core::CDimension2d<int> ViewPortSize;

		ox::core::CMatrix4 TransformationMatrix[ETS_COUNT];

		IK3DTriangleRenderer* CurrentTriangleRenderer;
		IK3DTriangleRenderer* TriangleRenderers[ETR_COUNT];
		ETriangleRenderer CurrentRenderer;

		IZBuffer* ZBuffer;

		ox::video::ITexture* Texture;
		ox::scene::SViewFrustrum Frustrum;

		SMaterial Material;

		splane planes[6]; // current planes of the view frustrum
	};

} // end namespace video
} // end namespace daisy


#endif
