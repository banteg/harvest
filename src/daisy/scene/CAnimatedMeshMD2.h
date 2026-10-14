// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshMD2.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_ANIMATED_MESH_MD2_H_INCLUDED__
#define __C_ANIMATED_MESH_MD2_H_INCLUDED__

#include "ox/scene/IAnimatedMeshMD2.h"
#include "ox/core/CAabbox3d.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/io/IReadFile.h"
#include "ox/video/S3DVertex.h"
#include "ox/TArray.h"
#include "ox/core/CString.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CAnimatedMeshMD2 : public IAnimatedMeshMD2, IMesh, IMeshBuffer
	{
	public:

		//! constructor
		CAnimatedMeshMD2();

		//! destructor
		virtual ~CAnimatedMeshMD2();

		//! loads an md2 file
		virtual bool loadFile(ox::io::IReadFile* file);

		//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
		virtual int getFrameCount();

		//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
		virtual IMesh* getMesh(int frame, int detailLevel=255, int startFrameLoop=-1, int endFrameLoop=-1);

		//! returns amount of mesh buffers.
		virtual int getMeshBufferCount();

		//! returns pointer to a mesh buffer
		virtual IMeshBuffer* getMeshBuffer(int nr);

		 //! returns the material of this meshbuffer
        virtual const ox::video::SMaterial& getMaterial() const;

		 //! returns the material of this meshbuffer
        virtual ox::video::SMaterial& getMaterial();

		//! returns pointer to vertices
		virtual const void* getVertices() const; 

		//! returns pointer to vertices
		virtual void* getVertices(); 

		//! returns which type of vertex data is stored.
		virtual ox::video::E_VERTEX_TYPE getVertexType() const;

		//! returns amount of vertices
		virtual int getVertexCount() const;

		//! returns pointer to Indices
		virtual const unsigned short* getIndices() const;

		//! returns pointer to Indices
		virtual unsigned short* getIndices();

		//! returns amount of indices
		virtual int getIndexCount() const;

		//! returns an axis aligned bounding box
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! returns an axis aligned bounding box
		virtual ox::core::CAabbox3d<float>& getBoundingBox();

		//! sets a flag of all contained materials to a new value
		virtual void setMaterialFlag(ox::video::E_MATERIAL_FLAG flag, bool newvalue);

		//! Returns the type of the animated mesh.
		virtual E_ANIMATED_MESH_TYPE getMeshType() const;

		//! Returns frame loop data for a special MD2 animation type.
		virtual void getFrameLoop(EMD2_ANIMATION_TYPE, 
			int& outBegin, int& outEnd, int& outFps) const;

		//! Returns frame loop data for a special MD2 animation type.
		virtual bool getFrameLoop(const char* name, 
			int& outBegin, int& outEnd, int& outFps) const;

		//! Returns amount of md2 animations in this file.
		virtual int getAnimationCount() const;

		//! Returns name of md2 animation. 
		//! \param nr: Zero based index of animation. 
		virtual const char* getAnimationName(int nr) const;

	private:

		//! returns max element
		inline int max(int a, int b);

		//! updates the interpolation buffer
		void updateInterpolationBuffer(int frame, int startFrame, int endFrame);

		//! calculates the bounding box
		virtual void calculateBoundingBox();

		//! calculates normals
		void calculateNormals();

		ox::TArray<unsigned short> Indices;
		ox::TArray<ox::video::S3DVertex> *FrameList;
		ox::TArray<ox::core::CAabbox3d<float> > BoxList;
		unsigned int FrameCount;
		int TriangleCount;
		ox::video::SMaterial Material;

		ox::TArray<ox::video::S3DVertex> InterpolateBuffer;
		ox::core::CAabbox3d<float> BoundingBox;

		struct SFrameData
		{
			ox::core::CString<char> name;
			int begin;
			int end;
			int fps;
		};

		ox::TArray< SFrameData > FrameData;
	};

} // end namespace scene
} // end namespace daisy

#endif

