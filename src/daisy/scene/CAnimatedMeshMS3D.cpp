// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshMS3D.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SMaterial.h"
#include "ox/video/SColor.h"
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
	int i=0;

	if (!file)
		return false;

	HasAnimation = false;

	// find file size

	size_t fileSize = file->getSize();

	// read whole file

	unsigned char* buffer = new unsigned char[fileSize];
	size_t read = (int)file->read(buffer, fileSize);
	if (read != fileSize)
	{
		delete [] buffer;
		os::Printer::log("Could not read full file. Loading failed", file->getFileName(), ox::event::ELL_ERROR);
		return false;
	}

	// read header

	const unsigned char *pPtr = (unsigned char*)((void*)buffer);
	MS3DHeader *pHeader = (MS3DHeader*)pPtr;
	pPtr += sizeof(MS3DHeader);

	if ( strncmp( pHeader->ID, "MS3D000000", 10 ) != 0 )
	{
		delete [] buffer;
		os::Printer::log("Not a valid Milkshape3D Model File. Loading failed", file->getFileName(), ox::event::ELL_ERROR);
		return false;
	}

	if ( pHeader->Version < 3 || pHeader->Version > 4 )
	{
		delete [] buffer;
		os::Printer::log("Only Milkshape3D version 1.3 and 1.4 is supported. Loading failed", file->getFileName(), ox::event::ELL_ERROR);
		return false;
	}

	// get pointers to data

	// vertices
	unsigned short numVertices = *(unsigned short*)pPtr;
	pPtr += sizeof(unsigned short);
	MS3DVertex *vertices = (MS3DVertex*)pPtr;
	pPtr += sizeof(MS3DVertex) * numVertices;

	// triangles
	unsigned short numTriangles = *(unsigned short*)pPtr;
	pPtr += sizeof(unsigned short);
	MS3DTriangle *triangles = (MS3DTriangle*)pPtr;
	pPtr += sizeof(MS3DTriangle) * numTriangles;

	// groups
	unsigned short numGroups = *(unsigned short*)pPtr;
	pPtr += sizeof(unsigned short);

	//skip groups
	for (i=0; i<numGroups; ++i)
	{
		pPtr += 33; // name and 1 byte flags
		unsigned short triangleCount = *(unsigned short*)pPtr;
		pPtr += sizeof(unsigned short);
		pPtr += sizeof(unsigned short) * triangleCount; // triangle indices

		pPtr += sizeof(char); // material index
	}

	// skip materials
	unsigned short numMaterials = *(unsigned short*)pPtr;
	pPtr += sizeof(unsigned short);
	MS3DMaterial *materials = (MS3DMaterial*)pPtr;
	pPtr += sizeof(MS3DMaterial) * numMaterials;

	// animation time
	float framesPerSecond = *(float*)pPtr;
	pPtr += sizeof(float) * 2; // fps and current time

	int frameCount = *(int*)pPtr;
	pPtr += sizeof(int);

	totalTime = (frameCount / framesPerSecond) * 1000.0f;

	int jointCount = *(unsigned short*)pPtr;
	pPtr += sizeof(unsigned short);

	// load joints
	SJoint t;

	for (i=0; i<jointCount; ++i)
	{
		MS3DJoint *pJoint = (MS3DJoint*)pPtr;
		pPtr += sizeof(MS3DJoint);
		Joints.push_back(t);
		SJoint& jnt = Joints[Joints.size()-1];

		jnt.Name = pJoint->Name;
		jnt.Index = i;
		jnt.Rotation.X = pJoint->Rotation[0];
		jnt.Rotation.Y = pJoint->Rotation[1];
		jnt.Rotation.Z = pJoint->Rotation[2];
		jnt.Translation.X = pJoint->Translation[0];
		jnt.Translation.Y = pJoint->Translation[1];
		jnt.Translation.Z = pJoint->Translation[2];
		jnt.ParentName = pJoint->ParentName;
		jnt.Parent = -1;

		if (pJoint->NumRotationKeyframes ||
			pJoint->NumTranslationKeyframes)
			HasAnimation = true;

		// get rotation keyframes
		int j;
		for	(j=0; j<pJoint->NumRotationKeyframes; ++j)
		{
			MS3DKeyframe* kf = (MS3DKeyframe*)pPtr;
			pPtr += sizeof(MS3DKeyframe);

			SKeyframe k;
			k.timeindex = kf->Time * 1000.0f;
			k.data.X = kf->Parameter[0];
			k.data.Y = kf->Parameter[1];
			k.data.Z = kf->Parameter[2];
			jnt.RotationKeys.push_back(k);
		}

		// get translation keyframes
		for	(j=0; j<pJoint->NumTranslationKeyframes; ++j)
		{
			MS3DKeyframe* kf = (MS3DKeyframe*)pPtr;
			pPtr += sizeof(MS3DKeyframe);

			SKeyframe k;
			k.timeindex = kf->Time * 1000.0f;
			k.data.X = kf->Parameter[0];
			k.data.Y = kf->Parameter[1];
			k.data.Z = kf->Parameter[2];
			jnt.TranslationKeys.push_back(k);
		}
	}

	//find parent of every joint
	int j;
	for (i=0; i<(int)Joints.size(); ++i)
		if (Joints[i].ParentName.size() != 0)
		{
			for (j=0; j<(int)Joints.size(); ++j)
				if (i != j && Joints[i].ParentName == Joints[j].Name)
				{
					Joints[i].Parent = j;
					break;
				}

			if (Joints[i].Parent == -1)
				os::Printer::log("Found joint in model without parent.", ox::event::ELL_WARNING);
		}

	// sets up all joints with initial rotation and translation
	for (i=0; i<(int)Joints.size(); ++i)
	{
		SJoint& jnt = Joints[i];

		jnt.RelativeTransformation.setRotationRadians(jnt.Rotation);
		jnt.RelativeTransformation.setTranslation(jnt.Translation);

		if (jnt.Parent == -1)
			jnt.AbsoluteTransformation = jnt.RelativeTransformation;
		else
		{
			jnt.AbsoluteTransformation = Joints[jnt.Parent].AbsoluteTransformation;
			jnt.AbsoluteTransformation *= jnt.RelativeTransformation;
		}
	}

	// create vertices and indices, attach them to the joints.
	ox::video::S3DVertex v;

	for (i=0; i<numTriangles; ++i)
	{
		int j;
		for (j = 0; j<3; ++j)
		{
			v.TCoords.X = triangles[i].S[j];
			v.TCoords.Y = triangles[i].T[j];
			v.Normal.X = triangles[i].VertexNormals[j][0];
			v.Normal.Y = triangles[i].VertexNormals[j][1];
			v.Normal.Z = triangles[i].VertexNormals[j][2];
			v.Color = ox::video::SColor(255, 255, 255, 255);
			v.Pos.X = vertices[triangles[i].VertexIndices[j]].Vertex[0];
			v.Pos.Y = vertices[triangles[i].VertexIndices[j]].Vertex[1];
			v.Pos.Z = vertices[triangles[i].VertexIndices[j]].Vertex[2];

			int boneid = vertices[triangles[i].VertexIndices[j]].BoneID;
			if (boneid>=0 && boneid<(int)Joints.size())
				Joints[boneid].VertexIds.push_back(Vertices.size());
			//else
			//{
			//	os::Debuginfo::print("no bone for vertex, bone", boneid);
			//}

			Vertices.push_back(v);
		}

		Indices.push_back(Vertices.size()-3);
		Indices.push_back(Vertices.size()-2);
		Indices.push_back(Vertices.size()-1);
	}

	// calculate bounding box
	if (!Vertices.empty())
		BoundingBox.reset(Vertices[0].Pos);

	for (i=0; i<(int)Vertices.size(); ++i)
		BoundingBox.addInternalPoint(Vertices[i].Pos);

	// inverse translate and rotate all vertices for making animation easier
	if (HasAnimation)
	for (i=0; i<(int)Joints.size(); ++i)
		for (int j=0; j<(int)Joints[i].VertexIds.size(); ++j)
		{
			Joints[i].AbsoluteTransformation.inverseTranslateVect(
				Vertices[Joints[i].VertexIds[j]].Pos);

			Joints[i].AbsoluteTransformation.inverseRotateVect(
				Vertices[Joints[i].VertexIds[j]].Pos);

			Joints[i].AbsoluteTransformation.inverseRotateVect(
				Vertices[Joints[i].VertexIds[j]].Normal);
		}

	AnimatedVertices = Vertices;

	delete [] buffer;
	return true;
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
