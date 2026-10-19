// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_ANIMATED_MESH_SCENE_NODE_H_INCLUDED__
#define __C_ANIMATED_MESH_SCENE_NODE_H_INCLUDED__

#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IAnimatedMeshMD2.h"
#include "ox/scene/IDummyTransformationSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/IShadowVolumeSceneNode.h"
#include "ox/TArray.h"
#include "ox/scene/IAnimatedMesh.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CAnimatedMeshSceneNode : public IAnimatedMeshSceneNode
	{
	public:

		//! constructor
		CAnimatedMeshSceneNode(IAnimatedMesh* mesh, ISceneNode* parent, ISceneManager* mgr,	int id,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

		//! destructor
		virtual ~CAnimatedMeshSceneNode();

		//! sets the current frame. from now on the animation is played from this frame.
		virtual void setCurrentFrame(int frame);

		//! OnPostRender() is called just after rendering the whole scene.
		virtual void OnPostRender(unsigned int timeMs);

		//! frame
		virtual void OnPreRender();

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! sets the frames between the animation is looped.
		//! the default is 0 - MaximalFrameCount of the mesh.
		virtual bool setFrameLoop(int begin, int end);

		//! sets the speed with witch the animation is played
		virtual void setAnimationSpeed(int framesPerSecond);

		//! returns the material based on the zero based index i. To get the amount
		//! of materials used by this scene node, use getMaterialCount().
		//! This function is needed for inserting the node into the scene hirachy on a
		//! optimal position for minimizing renderstate changes, but can also be used
		//! to directly modify the material of a scene node.
		virtual ox::video::SMaterial& getMaterial(int i);
		
		//! returns amount of materials used by this scene node.
		virtual int getMaterialCount();

		//! Creates shadow volume scene node as child of this node
		//! and returns a pointer to it. 
		virtual IShadowVolumeSceneNode* addShadowVolumeSceneNode(int id,
			bool zfailmethod=true, float infinity=10000.0f);

		//! Returns a pointer to a child node, wich has the same transformation as 
		//! the corrsesponding joint, if the mesh in this scene node is a ms3d mesh.
		virtual ISceneNode* getMS3DJointNode(const char* jointName);

		//! Removes a child from this scene node.
		//! Implemented here, to be able to remove the shadow properly, if there is one,
		//! or to remove attached childs.
		virtual bool removeChild(ISceneNode* child);

		//! Starts a MD2 animation. 
		virtual bool setMD2Animation(EMD2_ANIMATION_TYPE anim);

		//! Starts a special MD2 animation. 
		virtual bool setMD2Animation(const char* animationName);

		//! Returns the current displayed frame number.
		virtual int getFrameNr();

	private:

		ox::TArray<ox::video::SMaterial> Materials;
		ox::core::CAabbox3d<float> Box;
		IAnimatedMesh* Mesh;
		int BeginFrameTime;
		int StartFrame, EndFrame;
		int FramesPerSecond;

		IShadowVolumeSceneNode* Shadow;

		ox::TArray<IDummyTransformationSceneNode* > JointChildSceneNodes;
	};

} // end namespace scene
} // end namespace daisy

#endif

