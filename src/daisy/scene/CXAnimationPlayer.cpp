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
#include "ox/algo/CArrayFunctions.h"
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
	// create mesh
	addFrameToMesh(&Reader->getRootFrame());

	// recalulate box
	OriginalMesh.recalculateBoundingBox();

	// sort weights in joints
	for (int j=0; j<(int)Joints.size(); ++j)
		ox::algo::sort(Joints[j].Weights.begin(), Joints[j].Weights.end());

	// copy mesh
	AnimatedMesh = Manipulator->createMeshCopy(&OriginalMesh);

	// create and link animation data
	prepareAnimationData();

	// first animation
	animateSkeleton();
	modifySkin();
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
	// find original joint of vertex

	int jnr = getJointNumberFromName(frame->Name);
	if (jnr == -1)
		return;

	// weight per joint
	Joints[jnr].Weights.push_back(SWeightData(
		meshbuffernr,
		vtxidx,
		1.0f));

	// weight per vertex
	SVertexWeight& weight = Weights[meshbuffernr][vtxidx];
	weight.add(1.0f, jnr);
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
	// set animated vertices to zero
	for (int k=0; k<(int)Joints.size(); ++k)
	{
		for (int w=0; w<(int)Joints[k].Weights.size(); ++w)
		{
			SWeightData& wd = Joints[k].Weights[w];

#ifdef _XREADER_DEBUG
			if (wd.buffer >= AnimatedMesh->getMeshBufferCount() ||
				wd.vertex >= AnimatedMesh->getMeshBuffer(wd.buffer)->getVertexCount())
				os::Printer::log("CXAnimationPlayer: Invalid Weights", ox::event::ELL_INFORMATION);
#endif

			ox::video::S3DVertex* nv = (ox::video::S3DVertex*)AnimatedMesh->getMeshBuffer(wd.buffer)->getVertices();
            nv[wd.vertex].Pos.set(0,0,0);
		}
	}


#ifdef _XREADER_DEBUG
	bool somethingIsWrong = false;

	// check if all vertices are set to zero
	for (int mb=0; mb<AnimatedMesh->getMeshBufferCount(); ++mb)
	{
		ox::video::S3DVertex* v = (ox::video::S3DVertex*)AnimatedMesh->getMeshBuffer(mb)->getVertices();
		int c = AnimatedMesh->getMeshBuffer(mb)->getVertexCount();
		for (int vt=0; vt<c; ++vt)
			if (v[vt].Pos != ox::core::CVector3d<float>(0,0,0))
			{
				char tmp[255];
				sprintf(tmp, "CXAnimationPlayer: Vertex not null. Buf:%d Vtx:%d", mb, vt);
				// this can happen if something is wrong with the vertex weights,
				// not all vertices have a weight attached.
				os::Printer::log(tmp, ox::event::ELL_INFORMATION);
				somethingIsWrong = true;
			}
	}

	if (somethingIsWrong)
	{
		// write out mesh informations
		char tmp[255];
		sprintf(tmp, "CXAnimationPlayer: Meshbuffers:%d", AnimatedMesh->getMeshBufferCount());
		os::Printer::log(tmp, ox::event::ELL_INFORMATION);

		for (int mb=0; mb<AnimatedMesh->getMeshBufferCount(); ++mb)
		{
			sprintf(tmp, "CXAnimationPlayer: Meshbuffer #%d: %d vertices", mb, AnimatedMesh->getMeshBuffer(mb)->getVertexCount());
			os::Printer::log(tmp, ox::event::ELL_INFORMATION);
		}
	}
#endif

	// transform vertices
	for (int mb=0; mb<AnimatedMesh->getMeshBufferCount(); ++mb)
	{
		ox::video::S3DVertex* av = (ox::video::S3DVertex*)AnimatedMesh->getMeshBuffer(mb)->getVertices();
		ox::video::S3DVertex* ov = (ox::video::S3DVertex*)OriginalMesh.getMeshBuffer(mb)->getVertices();
		int c = AnimatedMesh->getMeshBuffer(mb)->getVertexCount();
		for (int vt=0; vt<c; ++vt)
		{
			ox::core::CVector3d<float> vtmp;
			ox::core::CVector3d<float> orig = ov[vt].Pos;
			SVertexWeight& weight = Weights[mb][vt];

			av[vt].Pos = ox::core::CVector3d<float>(0,0,0);

			/*if (weight.weightCount == 0)
			{
				av[vt].Pos = ov[vt].Pos;
			}
			else*/
			for (int w=0; w<weight.weightCount; ++w)
			{
				Joints[weight.joint[w]].CombinedAnimationMatrix.transformVect(
					orig, vtmp);

				/*ox::core::CMatrix4 mat;
				mat = Joints[weight.joint[w]].GlobalMatrix;
				mat *= Joints[weight.joint[w]].MatrixOffset;
				mat.transformVect(orig, vtmp);*/

				vtmp *= weight.weight[w];
				av[vt].Pos += vtmp;
			}


		}
	}

	// transform vertices
	/*ox::core::CVector3d<float> vtmp;
	for (int k=0; k<(int)Joints.size(); ++k)
	{

		for (int w=0; w<(int)Joints[k].Weights.size(); ++w)
		{
			SWeightData& wd = Joints[k].Weights[w];
			ox::video::S3DVertex* nv = (ox::video::S3DVertex*)AnimatedMesh->getMeshBuffer(wd.buffer)->getVertices();
			ox::video::S3DVertex* ov = (ox::video::S3DVertex*)OriginalMesh.getMeshBuffer(	wd.buffer)->getVertices();

			ox::core::CMatrix4 m = Joints[k].XFrame->GlobalMatrix;
			m *= Joints[k].MatrixOffset;

			m.transformVect(ov[wd.vertex].Pos, vtmp);

			vtmp *= wd.weight;
			nv[wd.vertex].Pos += vtmp;

		}
	}*/

}


//! prepares animation data which was read in from the .x file
void CXAnimationPlayer::prepareAnimationData()
{
    int animationSetCount = Reader->getAnimationSetCount();
	for (int i=0; i<animationSetCount; ++i)
	{
		AnimationSets.push_back(SXAnimationSet());
		SXAnimationSet& mySet = AnimationSets.back();

		CXFileReader::SXAnimationSet& readerSet = Reader->getAnimationSet(i);
		mySet.AnimationName = readerSet.AnimationName;

		// trough all animations
		for (int a=0; a<(int)readerSet.Animations.size(); ++a)
		{
			// trough all keys
			for (int k=0; k<(int)readerSet.Animations[a].Keys.size(); ++k)
			{
				// link with joint
				int jntnr = getJointNumberFromName(readerSet.Animations[a].FrameName);

				if (jntnr == -1)
				{
					os::Printer::log(
						"CXAnimationPlayer: Animationtrack without corresponding joint found",
						readerSet.Animations[a].FrameName.c_str(), ox::event::ELL_INFORMATION);
					continue;
				}

				// add new track
				mySet.Animations.push_back(SXAnimationTrack());
				SXAnimationTrack& myTrack = mySet.Animations.back();
				myTrack.jointNr = jntnr;
				IsAnimatedSkinnedMesh = true;

				// copy track
				int keyCount = (int)readerSet.Animations[a].Keys[k].numberOfKeys;
				int type = readerSet.Animations[a].Keys[k].keyType;
				int i;
				myTrack.keyType = type;

		#ifdef _XREADER_DEBUG
				char tmp[255];
				sprintf(tmp, "adding track %s with %d keys, type:%d", readerSet.Animations[a].FrameName.c_str(), keyCount, type);
				os::Printer::log(tmp, ox::event::ELL_INFORMATION);
		#endif

				switch(type)
				{
				case 0: // quaternion
					for (i=0; i<keyCount; ++i)
						myTrack.Quaternions.push_back(readerSet.Animations[a].Keys[k].getQuaternion(i));
					break;
				case 1: // scale
				case 2: // position
					for (i=0; i<keyCount; ++i)
						myTrack.Vectors.push_back(readerSet.Animations[a].Keys[k].getVector(i));
					break;
				case 4:
				case 3: // matrix
					for (i=0; i<keyCount; ++i)
						myTrack.Matrices.push_back(readerSet.Animations[a].Keys[k].getMatrix(i));
					break;
				}

				// copy times
				for (i=0; i<keyCount; ++i)
					myTrack.Times.push_back((float)readerSet.Animations[a].Keys[k].time[i]);

				if (myTrack.Times.back() > LastAnimationTime)
					LastAnimationTime = myTrack.Times.back();
			}

		}

		// sort animation tracks
		ox::algo::sort(mySet.Animations.begin(), mySet.Animations.end());
	}
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
