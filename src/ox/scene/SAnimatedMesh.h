// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SAnimatedMesh.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __S_ANIMATED_MESH_H_INCLUDED__
#define __S_ANIMATED_MESH_H_INCLUDED__

#include "IAnimatedMesh.h"
#include "ox/core/CAabbox3d.h"
#include "ox/scene/IMesh.h"
#include "ox/TArray.h"

namespace ox
{
namespace scene
{

	//! Simple implementation of the IAnimatedMesh interface.
	struct SAnimatedMesh : public IAnimatedMesh
	{
		//! constructor
		SAnimatedMesh()
		{

			Type = EAMT_UNKNOWN;
		}


		//! destructor
		~SAnimatedMesh()
		{
			// drop meshes
			for (unsigned int i=0; i<Meshes.size(); ++i)
				Meshes[i]->drop();
		};


		//! Gets the frame count of the animated mesh.
		//! \return Returns the amount of frames. If the amount is 1, it is a static, non animated mesh.
		virtual int getFrameCount()
		{
			return Meshes.size();
		}



		//! Returns the IMesh interface for a frame.
		//! \param frame: Frame number as zero based index. The maximum frame number is
		//! getFrameCount() - 1;
		//! \param detailLevel: Level of detail. 0 is the lowest,
		//! 255 the highest level of detail. Most meshes will ignore the detail level.
		//! \return Returns the animated mesh based on a detail level.
		virtual IMesh* getMesh(int frame, int detailLevel, int startFrameLoop=-1, int endFrameLoop=-1)
		{
			return Meshes[frame];
		}


		//! adds a Mesh
		void addMesh(IMesh* mesh)
		{
			if (mesh)
			{
				mesh->grab();
				Meshes.push_back(mesh);
			}
		}


		//! Returns an axis aligned bounding box of the mesh.
		//! \return ox::core::A bounding box of this mesh is returned.
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const
		{
			return Box;
		}

		void recalculateBoundingBox()
		{
			Box.reset(0,0,0);

			if (Meshes.empty())
				return;

			Box = Meshes[0]->getBoundingBox();

			for (unsigned int i=1; i<Meshes.size(); ++i)
				Box.addInternalBox(Meshes[i]->getBoundingBox());
		}


		//! Returns the type of the animated mesh.
		virtual E_ANIMATED_MESH_TYPE getMeshType() const
		{
			return Type;
		}

		ox::core::CAabbox3d<float> Box;
		ox::TArray<IMesh*> Meshes;
		E_ANIMATED_MESH_TYPE Type;
	};


} // end namespace scene
} // end namespace ox

#endif
