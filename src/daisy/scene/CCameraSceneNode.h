// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_CAMERA_SCENE_NODE_H_INCLUDED__
#define __C_CAMERA_SCENE_NODE_H_INCLUDED__

#include "ox/scene/ICameraSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/event/IEventReceiver.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CCameraSceneNode : public ICameraSceneNode
	{
	public:

		//! constructor
		CCameraSceneNode(ISceneNode* parent, ISceneManager* mgr, int id, 
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& lookat = ox::core::CVector3d<float>(0,0,100));

		//! destructor
		virtual ~CCameraSceneNode();

		//! Sets the projection matrix of the camera. The ox::core::CMatrix4 class has some methods
		//! to build a projection matrix. e.g: ox::core::CMatrix4::buildProjectionMatrixPerspectiveFovLH
		//! \param projection: The new projection matrix of the camera. 
		virtual void setProjectionMatrix(const ox::core::CMatrix4& projection);

		//! Gets the current projection matrix of the camera
		//! \return Returns the current projection matrix of the camera.
		virtual const ox::core::CMatrix4& getProjectionMatrix();

		//! Gets the current view matrix of the camera
		//! \return Returns the current view matrix of the camera.
		virtual const ox::core::CMatrix4& getViewMatrix();

		//! It is possible to send mouse and key events to the camera. Most cameras
		//! may ignore this input, but camera scene nodes which are created for 
		//! example with ox::scene::ISceneManager::addMayaCameraSceneNode or
		//! ox::scene::ISceneManager::addMeshViewerCameraSceneNode, may want to get this input
		//! for changing their position, look at target or whatever. 
		virtual bool OnEvent(ox::event::SEvent event);

		//! sets the look at target of the camera
		//! \param pos: Look at target of the camera.
		virtual void setTarget(const ox::core::CVector3d<float>& pos);

		//! Gets the current look at target of the camera
		//! \return Returns the current look at target of the camera
		virtual ox::core::CVector3d<float> getTarget() const;

		//! sets the up vector of the camera
		//! \param pos: New upvector of the camera.
		virtual void setUpVector(const ox::core::CVector3d<float>& pos);

		//! Gets the up vector of the camera.
		//! \return Returns the up vector of the camera.
		virtual ox::core::CVector3d<float> getUpVector() const;

		//! \return returns value of the near plane of the camera
		virtual float getNearValue();

		//! \return returns value of the far plane of the camera
		virtual float getFarValue();

		//! \return returns aspect ration of the camera
		virtual float getAspectRatio();

		//! \return returns field of view of the camera
		virtual float getFOV();

		//! sets the value of the near clipping plane. (default: 1.0f)
		virtual void setNearValue(float zn);

		//! sets the value of the far clipping plane (default: 2000.0f)
		virtual void setFarValue(float zf);

		//! sets the aspect ratio (default: 4.0f / 3.0f)
		virtual void setAspectRatio(float aspect);

		//! sets the field of view (Default: ox::core::PI / 3.5f)
		virtual void setFOV(float fovy);

		//! pre render event
		virtual void OnPreRender();

		//! post render event
		virtual void OnPostRender();

		//! render
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! returns the view area. needed sometimes by bsp or lod render nodes.
		virtual const SViewFrustrum* getViewFrustrum();

		//! Disables or enables the camera to get key or mouse inputs.
		//! If this is set to true, the camera will respond to key inputs
		//! otherwise not.
		virtual void setInputReceiverEnabled(bool enabled);

		//! Returns if the input receiver of the camera is currently enabled.
		virtual bool isInputReceiverEnabled();

	protected:

		void recalculateProjectionMatrix();
		void recalculateViewArea();

		//ox::core::CVector3d<float> Pos;
		ox::core::CVector3d<float> Target;
		ox::core::CVector3d<float> UpVector;

		ox::core::CMatrix4 Projection;
		ox::core::CMatrix4 View;

		ox::core::CAabbox3d<float> BBox;

		float fovy;		// Field of view, in radians. 
		float aspect;	// Aspect ratio. 
		float zn;		// value of the near view-plane. 
		float zf;		// ox::core::Z-value of the far view-plane.

		ox::core::CDimension2d<float> screenDim;

		SViewFrustrum ViewArea;

		bool InputReceiverEnabled;
	};

} // end namespace
} // end namespace

#endif

