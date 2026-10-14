// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshMS3D.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_ANIMATED_MESH_MS3D_H_INCLUDED__
#define __C_ANIMATED_MESH_MS3D_H_INCLUDED__

#include "ox/scene/IAnimatedMeshMS3D.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/TArray.h"
#include "ox/io/IReadFile.h"
#include "ox/video/S3DVertex.h"
#include "ox/core/CString.h"
#include "ox/core/CMatrix4.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CAnimatedMeshMS3D : public IAnimatedMeshMS3D, IMesh, IMeshBuffer
	{
	public:

		//! constructor
		CAnimatedMeshMS3D();

		//! destructor
		virtual ~CAnimatedMeshMS3D();

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

		//! Returns a pointer to a transformation matrix of a part of the 
		//! mesh based on a frame time.
		virtual ox::core::CMatrix4* getMatrixOfJoint(int jointNumber, int frame);

		//! Gets joint count.
		virtual int getJointCount() const;

		//! Gets the name of a joint.
		virtual const char* getJointName(int number) const;

		//! Gets a joint number from its name
		virtual int getJointNumber(const char* name) const;

	private:
	
		struct SKeyframe
		{
			float timeindex;
			ox::core::CVector3d<float> data; // translation or rotation
		};
	
		struct SJoint
		{
			ox::core::CString<char> Name;
			int Index;
			ox::core::CVector3d<float> Rotation;
			ox::core::CVector3d<float> Translation;
			
			ox::core::CMatrix4 RelativeTransformation;
			ox::core::CMatrix4 AbsoluteTransformation;
			ox::core::CMatrix4 AbsoluteTransformationAnimated;
			
			ox::TArray<SKeyframe> TranslationKeys;
			ox::TArray<SKeyframe> RotationKeys;
			ox::TArray<unsigned short> VertexIds;
			
			int Parent;
			ox::core::CString<char> ParentName;
		};

		void animate(int frame);
		void getKeyframeData(ox::TArray<SKeyframe>& keys, float time, ox::core::CVector3d<float>& outdata);

		ox::video::SMaterial Material;
		ox::core::CAabbox3d<float> BoundingBox;

		ox::TArray<ox::video::S3DVertex> Vertices;
		ox::TArray<ox::video::S3DVertex> AnimatedVertices;
		ox::TArray<unsigned short> Indices;
		
		ox::TArray<SJoint> Joints;
		
		float totalTime;
		bool HasAnimation;
		int lastCalculatedFrame;
	};

} // end namespace scene
} // end namespace daisy

#endif

