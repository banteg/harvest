// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CAnimatedMeshSceneNode.h"
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
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, SNRT_DEFAULT);

	ISceneNode::OnPreRender();

	if (IsVisible)
		for (int i=0; i<(int)JointChildSceneNodes.size(); ++i)
			if (JointChildSceneNodes[i])
				JointChildSceneNodes[i]->OnPreRender();
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
	int frameNr = getFrameNr();

	if (IsVisible)
	{
		// animate this node with all animators

		ox::TList<ISceneNodeAnimator*>::iterator ait = Animators.begin();
		for (; ait != Animators.end(); ++ait)
			(*ait)->animateNode(this, timeMs);


		// update all dummy transformation nodes
		if (!JointChildSceneNodes.empty() && Mesh && Mesh->getMeshType() == EAMT_MS3D)
		{
			IAnimatedMeshMS3D* amm = (IAnimatedMeshMS3D*)Mesh;
			ox::core::CMatrix4* mat;

			for (int i=0; i<(int)JointChildSceneNodes.size(); ++i)
				if (JointChildSceneNodes[i])
				{
					mat = amm->getMatrixOfJoint(i, frameNr);
					if (mat)
						JointChildSceneNodes[i]->getRelativeTransformationMatrix() = *mat;
				}
		}

		ox::TList<ISceneNode*>::iterator it = Children.begin();
		for (; it != Children.end(); ++it)
			(*it)->OnPostRender(timeMs);
	}
}


//! renders the node.
void CAnimatedMeshSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();

	if (!Mesh || !driver)
		return;

	driver->setTransform(ox::video::ETS_WORLD, AbsoluteTransformation);

	int frame = getFrameNr();

	ox::scene::IMesh* m = Mesh->getMesh(frame, 255, StartFrame, EndFrame);

	if (m)
	{
		Box = m->getBoundingBox();

		// for debug purposes only:
		if (DebugDataVisible)
		{
			ox::video::SMaterial m;
			m.Lighting = false;
			driver->setMaterial(m);
			driver->draw3DBox(Box, ox::video::SColor(0,255,255,255));

			if (Mesh->getMeshType() == EAMT_X)
			{
				// draw skeleton
				const ox::TArray<ox::core::CVector3d<float> >* ds =
					((IAnimatedMeshX*)Mesh)->getDrawableSkeleton(frame);

				for (int s=0; s<(int)ds->size(); s+=2)
					driver->draw3DLine((*ds)[s], (*ds)[s+1],  ox::video::SColor(0,255,255,255));
			}
		}

		if (Shadow)
			Shadow->setMeshToRenderFrom(m);

		for (int i=0; i<m->getMeshBufferCount(); ++i)
		{
			ox::scene::IMeshBuffer* mb = m->getMeshBuffer(i);
			driver->setMaterial(Materials[i]);
			driver->drawMeshBuffer(mb);
		}
	}
	#ifdef _DEBUG
	else
		os::Printer::log("Animated Mesh returned no mesh to render.", Mesh->getDebugName(), ox::event::ELL_WARNING);
	#endif
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
	if (!Mesh || Mesh->getMeshType() != EAMT_MS3D)
		return 0;

	IAnimatedMeshMS3D* amm = (IAnimatedMeshMS3D*)Mesh;
	int jointCount = amm->getJointCount();
	int number = amm->getJointNumber(jointName);

	if (number == -1)
	{
		os::Printer::log("Joint with specified name not found in ms3d mesh.", jointName, ox::event::ELL_WARNING);
		return 0;
	}

	if (JointChildSceneNodes.empty())
	{
		// allocate joints for the first time.
		JointChildSceneNodes.resize(jointCount);
		for (int i=0; i<jointCount; ++i)
			JointChildSceneNodes[i] = 0;
	}

	if (JointChildSceneNodes[number] == 0)
	{
		JointChildSceneNodes[number] =
			SceneManager->addDummyTransformationSceneNode(this, -1);
		JointChildSceneNodes[number]->grab();
	}

	return JointChildSceneNodes[number];
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
	if (!Mesh || Mesh->getMeshType() != EAMT_MD2)
		return false;

	IAnimatedMeshMD2* m = (IAnimatedMeshMD2*)Mesh;

	int begin, end, speed;
	m->getFrameLoop(anim, begin, end, speed);

	setAnimationSpeed(speed);
	setFrameLoop(begin, end);
	return true;
}


//! Starts a special MD2 animation.
bool CAnimatedMeshSceneNode::setMD2Animation(const char* animationName)
{
	if (!Mesh || Mesh->getMeshType() != EAMT_MD2)
		return false;

	IAnimatedMeshMD2* m = (IAnimatedMeshMD2*)Mesh;

	int begin, end, speed;
	if (!m->getFrameLoop(animationName, begin, end, speed))
		return false;

	setAnimationSpeed(speed);
	setFrameLoop(begin, end);
	return true;
}


} // end namespace scene
} // end namespace daisy
