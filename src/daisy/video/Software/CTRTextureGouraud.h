// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTRTextureGouraud.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_TRIANGLE_RENDERER_TEXTURE_GOURAUD_H_INCLUDED__
#define __C_TRIANGLE_RENDERER_TEXTURE_GOURAUD_H_INCLUDED__

#include "IK3DTriangleRenderer.h"
#include "ox/video/IImage.h"
#include "ox/core/CRect.h"

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;

	class CTRTextureGouraud : public IK3DTriangleRenderer
	{
	public:

		//! constructor
		CTRTextureGouraud(IZBuffer* zbuffer);
	
		//! destructor
		virtual ~CTRTextureGouraud();

		//! sets a render target
		virtual void setRenderTarget(ox::video::IImage* surface, const ox::core::CRect<int>& viewPort);

		//! draws an indexed triangle list
		virtual void drawIndexedTriangleList(S2DVertex* vertices, int vertexCount, const unsigned short* indexList, int triangleCount);

		//! en or disables the backface culling
		virtual void setBackfaceCulling(bool enabled = true);

		//! sets the Texture
		virtual void setTexture(ox::video::IImage* texture);

	protected:

		//! vertauscht zwei vertizen
		inline void swapVertices(const S2DVertex** v1, const S2DVertex** v2)
		{
			const S2DVertex* b = *v1;
			*v1 = *v2;
			*v2 = b;
		}

		ox::video::IImage* RenderTarget;
		ox::core::CRect<int> ViewPortRect;

		IZBuffer* ZBuffer;

		int SurfaceWidth;
		int SurfaceHeight;
		bool BackFaceCullingEnabled;
		TZBufferType* lockedZBuffer;
		short* lockedSurface;
		short* lockedTexture;
		int lockedTextureWidth;
		int textureXMask, textureYMask;
		ox::video::IImage* Texture;
	};

} // end namespace video
} // end namespace daisy

#endif

