// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGeometryCreator.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/video/ITexture.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/irrMath.h"
#include "ox/video/IImage.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/event/ILogger.h"
#include "CGeometryCreator.h"
#include "ox/scene/SAnimatedMesh.h"
#include "ox/scene/SMeshBuffer.h"
#include "ox/scene/SMesh.h"
#include "ox/video/IVideoDriver.h"
#include "daisy/video/Null/CImage.h"
#include <stdio.h>
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

// creates a hill plane
IAnimatedMesh* CGeometryCreator::createHillPlaneMesh(const ox::core::CDimension2d<float>& tileSize, const ox::core::CDimension2d<int>& tc,
	ox::video::SMaterial* material,	float hillHeight, const ox::core::CDimension2d<float>& ch,
	const ox::core::CDimension2d<float>& textureRepeatCount)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



IAnimatedMesh* CGeometryCreator::createTerrainMesh(ox::video::IImage* texture,
		ox::video::IImage* heightmap, const ox::core::CDimension2d<float>& stretchSize,
		float maxHeight, ox::video::IVideoDriver* driver,
		const ox::core::CDimension2d<int> maxVtxBlockSize,
		bool debugBorders)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


} // end namespace scene
} // end namespace daisy
