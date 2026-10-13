// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SMesh.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __S_MESH_H_INCLUDED__
#define __S_MESH_H_INCLUDED__

#include "IMeshBuffer.h"
#include "ox/core/CAabbox3d.h"
#include "ox/video/SMaterial.h"
#include "ox/TArray.h"
#include "IMesh.h"

namespace ox
{
namespace scene
{
	//! Simple implementation of the IMesh interface.
	struct SMesh : public IMesh
	{
		//! constructor
		SMesh()
		{
		}

		//! destructor
		~SMesh()
		{
			// drop buffers
			for (unsigned int i=0; i<MeshBuffers.size(); ++i)
				MeshBuffers[i]->drop();
		};

		//! returns amount of mesh buffers.
		virtual int getMeshBufferCount()
		{
			return MeshBuffers.size();
		}

		//! returns pointer to a mesh buffer
		virtual IMeshBuffer* getMeshBuffer(int nr)
		{
			return MeshBuffers[nr];
		}

		//! returns an axis aligned bounding box
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const
		{
			return BoundingBox;
		}

		//! Returns an axis aligned bounding box of the mesh.
		//! \return ox::core::A bounding box of this mesh is returned.
		virtual ox::core::CAabbox3d<float>& getBoundingBox()
		{
			return BoundingBox;
		}

		//! recalculates the bounding box
		void recalculateBoundingBox()
		{
			if (MeshBuffers.size())
			{
				BoundingBox = MeshBuffers[0]->getBoundingBox();
				for (unsigned int i=1; i<MeshBuffers.size(); ++i)
					BoundingBox.addInternalBox(MeshBuffers[i]->getBoundingBox());
			}
			else
				BoundingBox.reset(0.0f, 0.0f, 0.0f);
		}

		//! adds a MeshBuffer
		void addMeshBuffer(IMeshBuffer* buf)
		{
			if (buf)
			{
				buf->grab();
				MeshBuffers.push_back(buf);
			}
		}

		//! sets a flag of all contained materials to a new value
		virtual void setMaterialFlag(ox::video::E_MATERIAL_FLAG flag, bool newvalue)
		{
			for (unsigned int i=0; i<MeshBuffers.size(); ++i)
				MeshBuffers[i]->getMaterial().Flags[flag] = newvalue;
		}

		ox::TArray<IMeshBuffer*> MeshBuffers;
		ox::core::CAabbox3d<float> BoundingBox;
	};


} // end namespace scene
} // end namespace ox

#endif

