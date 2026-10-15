// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CStaticMeshOBJ.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_STATIC_MESH_OBJ_H_INCLUDED__
#define __C_STATIC_MESH_OBJ_H_INCLUDED__

#include "ox/scene/IAnimatedMesh.h"
#include "ox/core/CAabbox3d.h"
#include "ox/scene/IMesh.h"
#include "ox/io/IReadFile.h"
#include "ox/video/S3DVertex.h"
#include "ox/scene/SMesh.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CStaticMeshOBJ : public IAnimatedMesh
	{
	public:

		//! constructor
		CStaticMeshOBJ();

		//! destructor
		virtual ~CStaticMeshOBJ();

		//! loads an obj file
		virtual bool loadFile(ox::io::IReadFile* file);

		//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
		virtual int getFrameCount();

		//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
		virtual IMesh* getMesh(int frame, int detailLevel=255, int startFrameLoop=-1, int endFrameLoop=-1);

		//! Returns an axis aligned bounding box of the mesh.
		//! \return ox::core::A bounding box of this mesh is returned.
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! Returns the type of the animated mesh.
		virtual E_ANIMATED_MESH_TYPE getMeshType() const;

	private:

		char* getFirstWord(char* buf);
		void copyWord(char* outBuf, int bufLength, char* word);
		void copyWordLineEnd(char* outBuf, int bufLength, char* word);
		void copyWordSpaceEnd(char* outBuf, int bufLength, char* word);
		char* getNextWord(char* word);

		ox::scene::SMesh Mesh;
	};

} // end namespace scene
} // end namespace daisy

#endif
