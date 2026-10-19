// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGeometryCreator.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_GEOMETRY_CREATOR_H_INCLUDED__
#define __C_GEOMETRY_CREATOR_H_INCLUDED__

#include "ox/scene/IAnimatedMesh.h"
#include "ox/core/CDimension2d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/SMaterial.h"
#include "ox/video/IImage.h"



namespace ox { namespace video {} }
namespace ox { namespace scene {} }
namespace daisy
{
namespace video
{
using namespace ox::video;
}

namespace scene
{
using namespace ox::scene;

//! class for creating geometry on the fly
class CGeometryCreator
{
public:

	static IAnimatedMesh* createHillPlaneMesh(
		const ox::core::CDimension2d<float>& tileSize, const ox::core::CDimension2d<int>& tileCount,
		ox::video::SMaterial* material,	float hillHeight, const ox::core::CDimension2d<float>& countHills,
		const ox::core::CDimension2d<float>& textureRepeatCount);

	static IAnimatedMesh* createTerrainMesh(ox::video::IImage* texture,
		ox::video::IImage* heightmap, const ox::core::CDimension2d<float>& stretchSize, float maxHeight,
		ox::video::IVideoDriver* driver,
		const ox::core::CDimension2d<int> defaultVertexBlockSize,
		bool debugBorders=false);
};


} // end namespace scene
} // end namespace daisy

#endif
