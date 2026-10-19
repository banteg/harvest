// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CXAnimationPlayer.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CQuaternion.h"
#include "ox/core/CString.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "CXAnimationPlayer.h"
#include "ox/scene/ISceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "daisy/os.h"
#include "ox/scene/SMeshBuffer.h"
#include "ox/scene/IMeshManipulator.h"

#include <stdio.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CXAnimationPlayer::CXAnimationPlayer(CXFileReader* reader,
									 ox::video::IVideoDriver* driver,
									 IMeshManipulator* manip,
									 const char* filename)
: Reader(reader), Driver(driver), AnimatedMesh(0),
Manipulator(manip), CurrentAnimationTime(0.0f), LastAnimationTime(1.0f),
	CurrentAnimationSet(0), IsAnimatedSkinnedMesh(false), DebugSkeletonCrossSize(1.0f)
{
	FileName = filename;

	if (!Reader)
		return;

	if (Driver)
		Driver->grab();

	if (Manipulator)
		Manipulator->grab();

	Reader->grab();

	createAnimationData();
}



//! destructor
CXAnimationPlayer::~CXAnimationPlayer()
{
	if (Reader)
		Reader->drop();

	if (Driver)
		Driver->drop();

	if (AnimatedMesh)
		AnimatedMesh->drop();

	if (Manipulator)
		Manipulator->drop();
}



//! Gets the frame count of the animated mesh.
int CXAnimationPlayer::getFrameCount()
{
	return IsAnimatedSkinnedMesh ? (int)LastAnimationTime : 1;
}



//! Returns the IMesh interface for a frame.
IMesh* CXAnimationPlayer::getMesh(int frame, int detailLevel, int startFrameLoop, int endFrameLoop)
{
	if (!IsAnimatedSkinnedMesh)
		return &OriginalMesh;

	CurrentAnimationTime = (float)frame;
	animateSkeleton();
	modifySkin();
	updateBoundingBoxFromAnimation();
	return AnimatedMesh;
}



//! Returns an axis aligned bounding box of the mesh.
const ox::core::CAabbox3d<float>& CXAnimationPlayer::getBoundingBox() const
{
	return Box;
}



//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CXAnimationPlayer::getMeshType() const
{
	return EAMT_X;
}



void CXAnimationPlayer::createAnimationData()
{
	// get joints from x-file
	createJointData(&Reader->getRootFrame(), -1);

	createMeshData();

	if (IsAnimatedSkinnedMesh && AnimatedMesh)
	{
		animateSkeleton();
		modifySkin();	
		updateBoundingBoxFromAnimation();
		DebugSkeletonCrossSize = AnimatedMesh->getBoundingBox().getExtent().X / 20.0f;
	}
	else
		DebugSkeletonCrossSize = OriginalMesh.getBoundingBox().getExtent().X / 20.0f;
}


void CXAnimationPlayer::createMeshData()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


void CXAnimationPlayer::addFrameToMesh(CXFileReader::SXFrame* frame)
{
	// go through all meshes
	for (unsigned int m=0; m<frame->Meshes.size(); ++m)
	{
		// create mesh buffer for every material
		if (frame->Meshes[m].MaterialList.Materials.empty())
			os::Printer::log("Mesh without material found in x file.", ox::event::ELL_WARNING);

		for (unsigned int mt=0; mt<frame->Meshes[m].MaterialList.Materials.size(); ++mt)
		{
			// create buffer
			SMeshBuffer *buf = new SMeshBuffer();
			OriginalMesh.addMeshBuffer(buf);
			buf->drop();

			// new weights buffer
			Weights.push_back( ox::TArray<SVertexWeight>() );

			// create material
			buf->Material = getMaterialFromXMaterial(
				frame->Meshes[m].MaterialList.Materials[mt]);

			// add all faces of this material
			addFacesToBuffer(OriginalMesh.MeshBuffers.size()-1,
				frame->Meshes[m], mt, frame);
			buf->recalculateBoundingBox();
		}
	}

	// add child frames
	for (unsigned int c=0; c<frame->ChildFrames.size(); ++c)
		addFrameToMesh(&frame->ChildFrames[c]);
}



ox::video::SMaterial CXAnimationPlayer::getMaterialFromXMaterial(CXFileReader::SXMaterial& xmat)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



void CXAnimationPlayer::addFacesToBuffer(int meshbuffernr, CXFileReader::SXMesh& mesh, int matnr, CXFileReader::SXFrame* frame)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


void CXAnimationPlayer::addVirtualWeight(int meshbuffernr, int vtxidx, 
										CXFileReader::SXMesh& mesh,
										CXFileReader::SXFrame* frame)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



int CXAnimationPlayer::getJointNumberFromName(ox::core::CString<char>& name)
{
	for (int i=0; i<(int)Joints.size(); ++i)
		if (Joints[i].Name == name)
			return i;

	return -1;
}



ox::core::CString<char> CXAnimationPlayer::getTextureFileName(ox::core::CString<char> texture)
{
	int idx = -1;
	idx = FileName.findLast('/');

    if (idx == -1)
		idx = FileName.findLast('\\');

	if (idx == -1)
		return ox::core::CString<char>();

	ox::core::CString<char> p = FileName.subString(0, idx+1);
	p.append(texture);
	return p;
}



void CXAnimationPlayer::createJointData(CXFileReader::SXFrame* f, int JointParent)
{
	// add joint

	int index = Joints.size();
	Joints.push_back(SJoint());
	SJoint& j = Joints.back();

	j.Parent = JointParent;
	j.GlobalMatrix = f->GlobalMatrix;
	j.LocalMatrix = f->LocalMatrix;
	j.AnimatedMatrix = j.GlobalMatrix;
	j.LocalAnimatedMatrix = j.LocalMatrix;
	j.CombinedAnimationMatrix = j.AnimatedMatrix * j.MatrixOffset;
	j.IsVirtualJoint = false;

	j.Name = f->Name;

	// add all children
	for (int i=0; i<(int)f->ChildFrames.size(); ++i)
		createJointData(&f->ChildFrames[i], index);
}


//! Returns a pointer to a transformation matrix
ox::core::CMatrix4* CXAnimationPlayer::getMatrixOfJoint(int jointNumber, int frame)
{
	if (jointNumber < 0 || jointNumber >= (int)Joints.size())
		return 0;

	return &Joints[jointNumber].CombinedAnimationMatrix;
}


//! Gets joint count.
int CXAnimationPlayer::getJointCount() const
{
	return Joints.size();
}


//! Gets the name of a joint.
const char* CXAnimationPlayer::getJointName(int number) const
{
	if (number<0 || number>=(int)Joints.size())
		return 0;

	return Joints[number].Name.c_str();
}


//! Gets a joint number from its name
int CXAnimationPlayer::getJointNumber(const char* name) const
{
	for (int i=0; i<(int)Joints.size(); ++i)
		if (Joints[i].Name == name)
			return i;

	return -1;
}


//! Returns a pointer to list of points containing the skeleton.
const ox::TArray<ox::core::CVector3d<float> >* CXAnimationPlayer::getDrawableSkeleton(int frame)
{
	DebugSkeleton.clear();
	
	float k = DebugSkeletonCrossSize;
	float p = 0.0f;
	
	for (int i=0; i<(int)Joints.size(); ++i)
	{
		ox::core::CVector3d<float> start(p,p,p);
		ox::core::CVector3d<float> end(p,p,p);

		Joints[i].AnimatedMatrix.transformVect(start);

		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(0,k,0));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(0,-k,0));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(k,0,0));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(-k,0,0));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(0,0,k));
		DebugSkeleton.push_back(start);
		DebugSkeleton.push_back(start + ox::core::CVector3d<float>(0,0,-k));

		if (Joints[i].Parent != -1)
		{
			Joints[Joints[i].Parent].AnimatedMatrix.transformVect(end);

			DebugSkeleton.push_back(end);
			DebugSkeleton.push_back(start);
		}
	}

	return &DebugSkeleton;
}


//! animates the skeleton based on the animation data
void CXAnimationPlayer::animateSkeleton()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! modifies the skin based on the animated skeleton
void CXAnimationPlayer::modifySkin()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! prepares animation data which was read in from the .x file
void CXAnimationPlayer::prepareAnimationData()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}

void CXAnimationPlayer::updateBoundingBoxFromAnimation()
{
	if (!Joints.size())
		return;

	bool first = true;

	for (int i=1; i<(int)Joints.size(); ++i)
		if (!Joints[i].Weights.empty())
		{
			ox::core::CVector3d<float> p(0,0,0);
			Joints[i].AnimatedMatrix.transformVect(p);

			if (first)
				Box.reset(p);
			else
				Box.addInternalPoint(p);

			first = false;
		}

	AnimatedMesh->BoundingBox = Box;

}

//! Returns amount of animations in .X-file.
int CXAnimationPlayer::getAnimationCount() const
{
	return AnimationSets.size();
}

//! Returns the name of an animation. 
const char* CXAnimationPlayer::getAnimationName(int idx) const
{
	if (idx < 0 || idx >= (int)AnimationSets.size())
		return 0;

	return AnimationSets[idx].AnimationName.c_str();
}


//! Sets an animation as animation to play back.
void CXAnimationPlayer::setCurrentAnimation(int idx)
{
	if (idx < 0 || idx >= (int)AnimationSets.size())
		return;

	CurrentAnimationSet = idx;
}


//! Sets an animation as animation to play back.
bool CXAnimationPlayer::setCurrentAnimation(const char* name)
{
	for (int i=0; i<(int)AnimationSets.size(); ++i)
		if (AnimationSets[i].AnimationName == name)
		{
			CurrentAnimationSet = i;
			return true;
		}

	return false;
}


} // end namespace scene
} // end namespace daisy

