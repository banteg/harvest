// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/S2DVertex.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __S_K_2D_VERTEX_H_INCLUDED__
#define __S_K_2D_VERTEX_H_INCLUDED__

#include "ox/core/CVector2d.h"

typedef signed short TZBufferType;

namespace ox { namespace video {} }
namespace daisy
{
namespace video
{
using namespace ox::video;

	struct S2DVertex
	{
		ox::core::CVector2d<int> Pos;		// position
		ox::core::CVector2d<int> TCoords;	// texture coordinates
		TZBufferType ZValue;			// zvalue
		short Color;
	};


} // end namespace video
} // end namespace daisy

#endif

