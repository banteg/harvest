// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTriangleBBSelector.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_TRIANGLE_BB_SELECTOR_H_INCLUDED__
#define __C_TRIANGLE_BB_SELECTOR_H_INCLUDED__

#include "CTriangleSelector.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CTriangle3d.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Stupid triangle selector without optimization
class CTriangleBBSelector : public CTriangleSelector
{
public:

	//! Constructs a selector based on a mesh
	CTriangleBBSelector(ISceneNode* node);

	virtual ~CTriangleBBSelector() {};

	//! Gets all triangles.
	void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize, int& outTriangleCount, 
		const ox::core::CMatrix4* transform=0);
};

} // end namespace scene
} // end namespace daisy


#endif

