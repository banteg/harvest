// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/IK3DTriangleRenderer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __I_K_3D_TRIANGLE_RENDERER_H_INCLUDED__
#define __I_K_3D_TRIANGLE_RENDERER_H_INCLUDED__

#include "ox/IUnknown.h"
#include "ox/video/IImage.h"
#include "S2DVertex.h"
#include "ox/core/CRect.h"
#include "IZBuffer.h"
#include "ox/video/IImage.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;

	enum ETriangleRenderer
	{
		ETR_FLAT = 0,
		ETR_FLAT_WIRE,
		ETR_GOURAUD,
		ETR_GOURAUD_WIRE,
		ETR_TEXTURE_FLAT,
		ETR_TEXTURE_FLAT_WIRE,
		ETR_TEXTURE_GOURAUD,
		ETR_TEXTURE_GOURAUD_WIRE,
		ETR_COUNT
	};

	class IK3DTriangleRenderer : public ox::IUnknown
	{
	public:
	
		//! destructor
		virtual ~IK3DTriangleRenderer() {};

		//! sets a render target
		virtual void setRenderTarget(ox::video::IImage* surface, const ox::core::CRect<int>& viewPort) = 0;

		//! en or disables the backface culling
		virtual void setBackfaceCulling(bool enabled = true) = 0;

		//! sets the Texture
		virtual void setTexture(ox::video::IImage* texture) = 0;

		//! draws an indexed triangle list
		virtual void drawIndexedTriangleList(S2DVertex* vertices, int vertexCount, const unsigned short* indexList, int triangleCount) = 0;
	};


	IK3DTriangleRenderer* createTriangleRendererTextureGouraud(IZBuffer* zbuffer);
	IK3DTriangleRenderer* createTriangleRendererTextureGouraudWire(IZBuffer* zbuffer);
	IK3DTriangleRenderer* createTriangleRendererGouraud(IZBuffer* zbuffer);
	IK3DTriangleRenderer* createTriangleRendererGouraudWire(IZBuffer* zbuffer);
	IK3DTriangleRenderer* createTriangleRendererTextureFlat(IZBuffer* zbuffer);
	IK3DTriangleRenderer* createTriangleRendererTextureFlatWire(IZBuffer* zbuffer);
	IK3DTriangleRenderer* createTriangleRendererFlat(IZBuffer* zbuffer);
	IK3DTriangleRenderer* createTriangleRendererFlatWire(IZBuffer* zbuffer);


} // end namespace video
} // end namespace daisy

#endif

