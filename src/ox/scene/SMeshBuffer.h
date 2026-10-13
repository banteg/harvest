// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SMeshBuffer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __S_MESH_BUFFER_H_INCLUDED__
#define __S_MESH_BUFFER_H_INCLUDED__

#include "ox/TArray.h"
#include "ox/core/CAabbox3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SMaterial.h"
#include "IMeshBuffer.h"

namespace ox
{
namespace scene
{
	//! Simple implementation of the IMeshBuffer interface with ox::video::S3DVertex vertices.
	struct SMeshBuffer : public IMeshBuffer
	{
		//! constructor
		SMeshBuffer()
		{
		}

		//! destructor
		~SMeshBuffer() {}; 

        //! returns the material of this meshbuffer
        virtual const ox::video::SMaterial& getMaterial() const
		{
			return Material;
		}

		//! returns the material of this meshbuffer
        virtual ox::video::SMaterial& getMaterial()
		{
			return Material;
		}

		//! returns pointer to vertices
		virtual const void* getVertices() const
		{
			return &Vertices[0];
		} 

		//! returns pointer to vertices
		virtual void* getVertices()
		{
			return &Vertices[0];
		} 

		//! returns amount of vertices
		virtual int getVertexCount() const
		{
			return Vertices.size();
		}

		//! returns pointer to Indices
		virtual const unsigned short* getIndices() const
		{
			return &Indices[0];
		}

		//! returns pointer to Indices
		virtual unsigned short* getIndices()
		{
			return &Indices[0];
		}

		//! returns amount of indices
		virtual int getIndexCount() const
		{
			return Indices.size();
		}

		//! returns an axis aligned bounding box
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const
		{
			return BoundingBox;
		}

		//! returns an axis aligned bounding box
		virtual ox::core::CAabbox3d<float>& getBoundingBox()
		{
			return BoundingBox;
		}

		//! recalculates the bounding box. should be called if the mesh changed.
		void recalculateBoundingBox()
		{
			if (Vertices.empty())
				BoundingBox.reset(0,0,0);
			else
			{
				BoundingBox.reset(Vertices[0].Pos);
				for (unsigned int i=1; i<Vertices.size(); ++i)
					BoundingBox.addInternalPoint(Vertices[i].Pos);
			}
		}

		//! returns which type of vertex data is stored.
		virtual ox::video::E_VERTEX_TYPE getVertexType() const
		{
			return ox::video::EVT_STANDARD;
		}

		ox::video::SMaterial Material;						//! material for this meshBuffer.
		ox::TArray<ox::video::S3DVertex> Vertices;		//! Array of vertices
		ox::TArray<unsigned short> Indices;	//! Array of the Indices.
		ox::core::CAabbox3d<float> BoundingBox;
	};

} // end namespace scene
} // end namespace ox

#endif

