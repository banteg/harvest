// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTriangleSelector.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_TRIANGLE_SELECTOR_H_INCLUDED__
#define __C_TRIANGLE_SELECTOR_H_INCLUDED__

#include "ox/scene/ITriangleSelector.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CTriangle3d.h"
#include "ox/scene/ISceneNode.h"
#include "ox/TArray.h"
#include "ox/scene/IMesh.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! Stupid triangle selector without optimization
class CTriangleSelector : public ITriangleSelector
{
public:

	//! Constructs a selector based on a mesh
	CTriangleSelector(ISceneNode* node);

	//! Constructs a selector based on a mesh
	CTriangleSelector(IMesh* mesh, ISceneNode* node);

	//! Constructs a selector based on a bounding box
	CTriangleSelector(ox::core::CAabbox3d<float> box, ISceneNode* node);

	~CTriangleSelector();

	//! Gets all triangles.
	void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize, int& outTriangleCount, 
		const ox::core::CMatrix4* transform=0);

	//! Gets all triangles which lie within a specific bounding box.
	void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize, int& outTriangleCount, 
		const ox::core::CAabbox3d<float>& box, const ox::core::CMatrix4* transform=0);

	//! Gets all triangles which have or may have contact with a 3d line.
	virtual void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
		int& outTriangleCount, const ox::core::CLine3d<float>& line, 
		const ox::core::CMatrix4* transform=0);

	//! Returns amount of all available triangles in this selector
	virtual int getTriangleCount() const;

protected:

	ISceneNode* SceneNode;
	ox::TArray<ox::core::CTriangle3d<float> > Triangles;
};

} // end namespace scene
} // end namespace daisy


#endif

