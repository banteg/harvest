// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IAnimatedMeshMD2.h"
#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/scene/IShadowVolumeSceneNode.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "ox/TList.h"
#include "CAnimatedMeshSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/S3DVertex.h"
#include "daisy/os.h"
#include "CShadowVolumeSceneNode.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/IAnimatedMeshMS3D.h"
#include "ox/scene/IAnimatedMeshX.h"
#include "ox/scene/IDummyTransformationSceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;



//! constructor
CAnimatedMeshSceneNode::CAnimatedMeshSceneNode(IAnimatedMesh* mesh, ISceneNode* parent, ISceneManager* mgr, int id,
			const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& rotation,	const ox::core::CVector3d<float>& scale)
: IAnimatedMeshSceneNode(parent, mgr, id, position, rotation, scale), Mesh(mesh),
	BeginFrameTime(0), StartFrame(0), EndFrame(0), FramesPerSecond(100),
	Shadow(0)
{
	BeginFrameTime = os::Timer::getTime();

	if (Mesh)
	{
		// get materials.
		Box = Mesh->getBoundingBox();

		IMesh* m = Mesh->getMesh(0,0);
		if (m)
		{
			ox::video::SMaterial mat;
			for (int i=0; i<m->getMeshBufferCount(); ++i)
			{
				IMeshBuffer* mb = m->getMeshBuffer(i);
				if (mb)
					mat = mb->getMaterial();

				Materials.push_back(mat);
			}
		}

		// get start and begin time

		StartFrame = 0;
		EndFrame = Mesh->getFrameCount();

		// grab the mesh

		Mesh->grab();
	}
}



//! destructor
CAnimatedMeshSceneNode::~CAnimatedMeshSceneNode()
{
	if (Mesh)
		Mesh->drop();

	if (Shadow)
		Shadow->drop();

	for (int i=0; i<(int)JointChildSceneNodes.size(); ++i)
		if (JointChildSceneNodes[i])
			JointChildSceneNodes[i]->drop();
}



//! sets the current frame. from now on the animation is played from this frame.
void CAnimatedMeshSceneNode::setCurrentFrame(int frame)
{
}


//! frame
void CAnimatedMeshSceneNode::OnPreRender()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


inline int CAnimatedMeshSceneNode::getFrameNr()
{
	int frame = 0;

	if (EndFrame - StartFrame)
		frame = StartFrame + ((int)((os::Timer::getTime() - BeginFrameTime)
			* (FramesPerSecond/1000.0f)) % (EndFrame - StartFrame));
	else
		frame = StartFrame;

	return frame;
}



//! OnPostRender() is called just after rendering the whole scene.
void CAnimatedMeshSceneNode::OnPostRender(unsigned int timeMs)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! renders the node.
void CAnimatedMeshSceneNode::render()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! sets the frames between the animation is looped.
//! the default is 0 - MaximalFrameCount of the mesh.
bool CAnimatedMeshSceneNode::setFrameLoop(int begin, int end)
{
	if (!Mesh)
		return false;

	int frameCount = Mesh->getFrameCount();

	if (!(begin <= end && begin < frameCount && end < frameCount))
		return false;

	StartFrame = begin;
	EndFrame = end;
	BeginFrameTime = os::Timer::getTime();

	return true;
}



//! sets the speed with witch the animation is played
void CAnimatedMeshSceneNode::setAnimationSpeed(int framesPerSecond)
{
	FramesPerSecond = framesPerSecond;
}




//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CAnimatedMeshSceneNode::getBoundingBox() const
{
	return Box;//Mesh ? Mesh->getBoundingBox() : Box;
}


//! returns the material based on the zero based index i. To get the amount
//! of materials used by this scene node, use getMaterialCount().
//! This function is needed for inserting the node into the scene hirachy on a
//! optimal position for minimizing renderstate changes, but can also be used
//! to directly modify the material of a scene node.
ox::video::SMaterial& CAnimatedMeshSceneNode::getMaterial(int i)
{
	if (i < 0 || i >= (int)Materials.size())
		return ISceneNode::getMaterial(i);

	return Materials[i];
}



//! returns amount of materials used by this scene node.
int CAnimatedMeshSceneNode::getMaterialCount()
{
	return Materials.size();
}


//! Creates shadow volume scene node as child of this node
//! and returns a pointer to it.
IShadowVolumeSceneNode* CAnimatedMeshSceneNode::addShadowVolumeSceneNode(int id,
																		 bool zfailmethod,
																		 float infinity)
{
	if (!SceneManager->getVideoDriver()->queryFeature(ox::video::EVDF_STENCIL_BUFFER))
		return 0;

	if (Shadow)
	{
		os::Printer::log("This node already has a shadow.", ox::event::ELL_WARNING);
		return 0;
	}

	Shadow = new CShadowVolumeSceneNode(this, SceneManager, -1,  zfailmethod, infinity);
	return Shadow;
}



//! Returns a pointer to a child node, wich has the same transformation as
//! the corrsesponding joint, if the mesh in this scene node is a ms3d mesh.
ISceneNode* CAnimatedMeshSceneNode::getMS3DJointNode(const char* jointName)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! Removes a child from this scene node.
//! Implemented here, to be able to remove the shadow properly, if there is one,
//! or to remove attached childs.
bool CAnimatedMeshSceneNode::removeChild(ISceneNode* child)
{
	if (child && Shadow == child)
	{
		Shadow->drop();
		Shadow = 0;
		return true;
	}

	if (ISceneNode::removeChild(child))
	{
		for (int i=0; i<(int)JointChildSceneNodes.size(); ++i)
		if (JointChildSceneNodes[i] == child)
		{
			JointChildSceneNodes[i]->drop();
			JointChildSceneNodes[i] = 0;
			return true;
		}

		return true;
	}

	return false;
}


//! Starts a MD2 animation.
bool CAnimatedMeshSceneNode::setMD2Animation(EMD2_ANIMATION_TYPE anim)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! Starts a special MD2 animation.
bool CAnimatedMeshSceneNode::setMD2Animation(const char* animationName)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


} // end namespace scene
} // end namespace daisy
