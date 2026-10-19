// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshMS3D.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/io/IReadFile.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "CAnimatedMeshMS3D.h"
#include "daisy/os.h"
#include <string.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


// byte-align structures
#ifdef _MSC_VER
#	pragma pack( push, packing )
#	pragma pack( 1 )
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error compiler not supported
#endif

// File header
struct MS3DHeader
{
	char ID[10];
	int Version;
} PACK_STRUCT;

// Vertex information
struct MS3DVertex
{
	unsigned char Flags;
	float Vertex[3];
	char BoneID;
	unsigned char RefCount;
} PACK_STRUCT;

// Triangle information
struct MS3DTriangle
{
	unsigned short Flags;
	unsigned short VertexIndices[3];
	float VertexNormals[3][3];
	float S[3], T[3];
	unsigned char SmoothingGroup;
	unsigned char GroupIndex;
} PACK_STRUCT;

// Material information
struct MS3DMaterial
{
    char Name[32];
    float Ambient[4];
    float Diffuse[4];
    float Specular[4];
    float Emissive[4];
    float Shininess;	// 0.0f - 128.0f
    float Transparency;	// 0.0f - 1.0f
    unsigned char Mode;	// 0, 1, 2 is unused now
    char Texture[128];
    char Alphamap[128];
} PACK_STRUCT;

//	Joint information
struct MS3DJoint
{
	unsigned char Flags;
	char Name[32];
	char ParentName[32];
	float Rotation[3];
	float Translation[3];
	unsigned short NumRotationKeyframes;
	unsigned short NumTranslationKeyframes;
} PACK_STRUCT;

// Keyframe data
struct MS3DKeyframe
{
	float Time;
	float Parameter[3];
} PACK_STRUCT;

// Default alignment
#ifdef _MSC_VER
#	pragma pack( pop, packing )
#endif

#undef PACK_STRUCT


//! constructor
CAnimatedMeshMS3D::CAnimatedMeshMS3D()
{
	lastCalculatedFrame = -1;
}



//! destructor
CAnimatedMeshMS3D::~CAnimatedMeshMS3D()
{
}



//! loads an md2 file
bool CAnimatedMeshMS3D::loadFile(ox::io::IReadFile* file)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
int CAnimatedMeshMS3D::getFrameCount()
{
	return (int)totalTime;
}



//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
IMesh* CAnimatedMeshMS3D::getMesh(int frame, int detailLevel, int startFrameLoop, int endFrameLoop)
{
	animate(frame);
	return this;
}



void CAnimatedMeshMS3D::getKeyframeData(ox::TArray<SKeyframe>& keys, float time, ox::core::CVector3d<float>& outdata)
{
	for (int i=0; i<(int)keys.size()-1; ++i)
	{
		if (keys[i].timeindex <= time && keys[i+1].timeindex >= time)
		{
			float interpolate = (time - keys[i].timeindex)/(keys[i+1].timeindex - keys[i].timeindex);
			outdata = keys[i].data + ((keys[i+1].data - keys[i].data) * interpolate);
			return;
		}
	}
}


//! Returns a pointer to a transformation matrix of a part of the 
//! mesh based on a frame time.
ox::core::CMatrix4* CAnimatedMeshMS3D::getMatrixOfJoint(int jointNumber, int frame)
{
	if (!HasAnimation || jointNumber < 0 || jointNumber >= (int)Joints.size())
		return 0;

	animate(frame);

	return &Joints[jointNumber].AbsoluteTransformationAnimated;
}


//! Gets joint count.
int CAnimatedMeshMS3D::getJointCount() const
{
	return Joints.size();
}


//! Gets the name of a joint.
const char* CAnimatedMeshMS3D::getJointName(int number) const
{
	if (number < 0 || number >= (int)Joints.size())
		return 0;
	return Joints[number].Name.c_str();
}


//! Gets a joint number from its name
int CAnimatedMeshMS3D::getJointNumber(const char* name) const
{
	for (int i=0; i<(int)Joints.size(); ++i)
		if (Joints[i].Name == name)
			return i;

	return -1;
}


void CAnimatedMeshMS3D::animate(int frame)
{
	if (!HasAnimation || lastCalculatedFrame == frame)
		return;

	float time = (float)frame;
	ox::core::CMatrix4 transform;
	lastCalculatedFrame = frame;

	for (int i=0; i<(int)Joints.size(); ++i)
	{
		ox::core::CVector3d<float> translation = Joints[i].Translation;
		ox::core::CVector3d<float> rotation = Joints[i].Rotation;
		
		// find keyframe translation and roation
		getKeyframeData(Joints[i].TranslationKeys, time, translation);
		getKeyframeData(Joints[i].RotationKeys, time, rotation);

		transform.makeIdentity();
		transform.setRotationRadians(rotation);
		transform.setTranslation(translation);

		Joints[i].AbsoluteTransformationAnimated = Joints[i].RelativeTransformation * transform;

		if (Joints[i].Parent != -1)
			Joints[i].AbsoluteTransformationAnimated = Joints[Joints[i].Parent].AbsoluteTransformationAnimated * Joints[i].AbsoluteTransformationAnimated;

		if (i==0)
			BoundingBox.reset(Joints[0].AbsoluteTransformationAnimated.getTranslation());
		else
			BoundingBox.addInternalPoint(Joints[i].AbsoluteTransformationAnimated.getTranslation());


		// transform all vertices
		for (int j=0; j<(int)Joints[i].VertexIds.size(); ++j)
		{
			Joints[i].AbsoluteTransformationAnimated.transformVect(
			Vertices[Joints[i].VertexIds[j]].Pos, 
				AnimatedVertices[Joints[i].VertexIds[j]].Pos);

			// TODO: this could be done much more faster by 
			// first getting 8 extreme points and putting them in.
			BoundingBox.addInternalPoint(AnimatedVertices[Joints[i].VertexIds[j]].Pos);
		}
	}

}



//! returns amount of mesh buffers.
int CAnimatedMeshMS3D::getMeshBufferCount()
{
	return 1;
}



//! returns pointer to a mesh buffer
IMeshBuffer* CAnimatedMeshMS3D::getMeshBuffer(int nr)
{
	return this;
}



//! returns the material of this meshbuffer
const ox::video::SMaterial& CAnimatedMeshMS3D::getMaterial() const
{
	return Material;
}



//! returns the material of this meshbuffer
ox::video::SMaterial& CAnimatedMeshMS3D::getMaterial()
{
	return Material;
}



//! returns pointer to vertices
const void* CAnimatedMeshMS3D::getVertices() const
{
	return &AnimatedVertices[0];
}


//! returns pointer to vertices
void* CAnimatedMeshMS3D::getVertices()
{
	return &AnimatedVertices[0];
}


//! returns which type of vertex data is stored.
ox::video::E_VERTEX_TYPE CAnimatedMeshMS3D::getVertexType() const
{
	return ox::video::EVT_STANDARD;
}



//! returns amount of vertices
int CAnimatedMeshMS3D::getVertexCount() const
{
	return Vertices.size();
}



//! returns pointer to Indices
const unsigned short* CAnimatedMeshMS3D::getIndices() const
{
	return &Indices[0];
}


//! returns pointer to Indices
unsigned short* CAnimatedMeshMS3D::getIndices()
{
	return &Indices[0];
}



//! returns amount of indices
int CAnimatedMeshMS3D::getIndexCount() const
{
	return Indices.size();
}


//! returns an axis aligned bounding box
const ox::core::CAabbox3d<float>& CAnimatedMeshMS3D::getBoundingBox() const
{
	return BoundingBox;
}


//! returns an axis aligned bounding box
ox::core::CAabbox3d<float>& CAnimatedMeshMS3D::getBoundingBox()
{
	return BoundingBox;
}


//! sets a flag of all contained materials to a new value
void CAnimatedMeshMS3D::setMaterialFlag(ox::video::E_MATERIAL_FLAG flag, bool newvalue)
{
	Material.Flags[flag] = newvalue;
}


//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CAnimatedMeshMS3D::getMeshType() const
{
	return EAMT_MS3D;
}



} // end namespace scene
} // end namespace daisy

