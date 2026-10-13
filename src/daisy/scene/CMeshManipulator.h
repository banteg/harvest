// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMeshManipulator.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_MESH_MANIPULATOR_H_INCLUDED__
#define __C_MESH_MANIPULATOR_H_INCLUDED__

#include "ox/scene/IMeshManipulator.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/SMesh.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! An interface for easily manipulate meshes.
/** Scale, set alpha value, flip surfaces, and so on. This exists for fixing problems 
 with wrong imported or exported meshes quickly after loading. It is not intended for doing mesh
 modifications and/or animations during runtime.
*/
class CMeshManipulator : public IMeshManipulator
{
public:

	//! Constructor
	CMeshManipulator();

	//! destructor
	virtual ~CMeshManipulator();

	//! Flips the direction of surfaces. Changes backfacing triangles to frontfacing
	//! triangles and vice versa.
	//! \param mesh: Mesh on which the operation is performed.
	virtual void flipSurfaces(ox::scene::IMesh* mesh) const;

	//! Sets the alpha vertex color value of the whole mesh to a new value
	//! \param mesh: Mesh on which the operation is performed.
	virtual void setVertexColorAlpha(ox::scene::IMesh* mesh, int alpha) const;

	//! Recalculates all normals of the mesh.
	//! \param mesh: Mesh on which the operation is performed.
	virtual void recalculateNormals(ox::scene::IMesh* mesh) const;

	//! Scales the whole mesh.
	//! \param mesh: Mesh on which the operation is performed.
	virtual void scaleMesh(ox::scene::IMesh* mesh, const ox::core::CVector3d<float>& scale) const;

	//! Clones a static IMesh into a modifyable SMesh.
	virtual SMesh* createMeshCopy(ox::scene::IMesh* mesh) const;

	//! Creates a planar texture mapping on the mesh
	//! \param mesh: Mesh on which the operation is performed.
	//! \param resolution: resolution of the planar mapping. This is the value
	//! specifying which is the releation between world space and 
	//! texture coordinate space.
	virtual void makePlanarTextureMapping(ox::scene::IMesh* mesh, float resolution) const;

	//! Recalculates the bounding box for a meshbuffer
	virtual void recalculateBoundingBox(ox::scene::IMeshBuffer* buffer) const;

	//! Returns amount of polygons in mesh.
	virtual int getPolyCount(ox::scene::IMesh* mesh) const;

	//! Returns amount of polygons in mesh.
	virtual int getPolyCount(ox::scene::IAnimatedMesh* mesh) const;
};

} // end namespace scene
} // end namespace daisy


#endif

