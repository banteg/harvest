// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/core/irrMath.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/event/IEventReceiver.h"
#include "CCameraSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
CCameraSceneNode::CCameraSceneNode(ISceneNode* parent, ISceneManager* mgr, int id, 
	const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& lookat)
	: ICameraSceneNode(parent, mgr, id, position, ox::core::CVector3d<float>(0.0f, 0.0f, 0.0f),
			ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f)), InputReceiverEnabled(true)
{
	BBox.reset(0,0,0);

	// set default view

	UpVector.set(0.0f, 1.0f, 0.0f);
	Target.set(lookat);

	// set default projection

	fovy = ox::core::PI / 2.5f;	// Field of view, in radians. 
	aspect = 4.0f / 3.0f;	// Aspect ratio. 
	zn = 1.0f;				// value of the near view-plane. 
	zf = 3000.0f;			// ox::core::Z-value of the far view-plane. 

	ox::video::IVideoDriver* d = mgr->getVideoDriver();
	if (d)
	{
		screenDim.Width = (float)d->getScreenSize().Width;
		screenDim.Height = (float)d->getScreenSize().Height;
		aspect = screenDim.Height / screenDim.Width;
	}

	recalculateProjectionMatrix();
}



//! destructor
CCameraSceneNode::~CCameraSceneNode()
{
}


//! Disables or enables the camera to get key or mouse inputs.
void CCameraSceneNode::setInputReceiverEnabled(bool enabled)
{
	InputReceiverEnabled = enabled;
}


//! Returns if the input receiver of the camera is currently enabled.
bool CCameraSceneNode::isInputReceiverEnabled()
{
	return InputReceiverEnabled;
}


//! Sets the projection matrix of the camera. The ox::core::CMatrix4 class has some methods
//! to build a projection matrix. e.g: ox::core::CMatrix4::buildProjectionMatrixPerspectiveFovLH
//! \param projection: The new projection matrix of the camera. 
void CCameraSceneNode::setProjectionMatrix(const ox::core::CMatrix4& projection)
{
	Projection = projection;
}



//! Gets the current projection matrix of the camera
//! \return Returns the current projection matrix of the camera.
const ox::core::CMatrix4& CCameraSceneNode::getProjectionMatrix()
{
	return Projection;
}



//! Gets the current view matrix of the camera
//! \return Returns the current view matrix of the camera.
const ox::core::CMatrix4& CCameraSceneNode::getViewMatrix()
{
	return View;
}



//! It is possible to send mouse and key events to the camera. Most cameras
//! may ignore this input, but camera scene nodes which are created for 
//! example with ox::scene::ISceneManager::addMayaCameraSceneNode or
//! ox::scene::ISceneManager::addMeshViewerCameraSceneNode, may want to get this input
//! for changing their position, look at target or whatever. 
bool CCameraSceneNode::OnEvent(ox::event::SEvent event)
{
	return false;
}



//! sets the look at target of the camera
//! \param pos: Look at target of the camera.
void CCameraSceneNode::setTarget(const ox::core::CVector3d<float>& pos)
{
	Target = pos;
}



//! Gets the current look at target of the camera
//! \return Returns the current look at target of the camera
ox::core::CVector3d<float> CCameraSceneNode::getTarget() const
{
	return Target;
}



//! sets the up vector of the camera
//! \param pos: New upvector of the camera.
void CCameraSceneNode::setUpVector(const ox::core::CVector3d<float>& pos)
{
	UpVector = pos;
}



//! Gets the up vector of the camera.
//! \return Returns the up vector of the camera.
ox::core::CVector3d<float> CCameraSceneNode::getUpVector() const
{
	return UpVector;
}


float CCameraSceneNode::getNearValue()
{
	return zn;
}

float CCameraSceneNode::getFarValue()
{
	return zf;
}

float CCameraSceneNode::getAspectRatio()
{
	return aspect;
}

float CCameraSceneNode::getFOV()
{
	return fovy;
}

void CCameraSceneNode::setNearValue(float f)
{
	zn = f;
	recalculateProjectionMatrix();
}

void CCameraSceneNode::setFarValue(float f)
{
	zf = f;
	recalculateProjectionMatrix();
}

void CCameraSceneNode::setAspectRatio(float f)
{
	aspect = f;
	recalculateProjectionMatrix();
}

void CCameraSceneNode::setFOV(float f)
{
	fovy = f;
	recalculateProjectionMatrix();
}

void CCameraSceneNode::recalculateProjectionMatrix()
{
	Projection.buildProjectionMatrixPerspectiveFovLH(fovy, aspect, zn, zf);
	//recalculateViewArea();
}


//! prerender
void CCameraSceneNode::OnPreRender()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (!driver)
		return;

	if (SceneManager->getActiveCamera() == this)
	{
		screenDim.Width = (float)driver->getScreenSize().Width;
		screenDim.Height = (float)driver->getScreenSize().Height;

		driver->setTransform(ox::video::ETS_PROJECTION, Projection);

		// if upvector and vector to the target are the same, we have a
		// problem. so solve this problem:

		ox::core::CVector3d<float> pos = getAbsolutePosition();
		ox::core::CVector3d<float> tgtv = Target - pos;
		tgtv.normalize();

		ox::core::CVector3d<float> up = UpVector;
		up.normalize();

		float dp = tgtv.dotProduct(up);
		if ((dp > -1.0001f && dp < -0.9999f) ||
			(dp < 1.0001f && dp > 0.9999f))
			up.X += 1.0f;

		View.buildCameraLookAtMatrixLH(pos, Target, up);
		recalculateViewArea();

		SceneManager->registerNodeForRendering(this, SNRT_LIGHT_AND_CAMERA);
	}

	ISceneNode::OnPreRender();
}


//! post render event
void CCameraSceneNode::OnPostRender()
{
	ISceneNode::OnPostRender(os::Timer::getTime());
}



//! render
void CCameraSceneNode::render()
{	
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (!driver)
		return;

	driver->setTransform(ox::video::ETS_VIEW, View);
}


//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CCameraSceneNode::getBoundingBox() const
{
	return BBox;
}



//! returns the view frustrum. needed sometimes by bsp or lod render nodes.
const SViewFrustrum* CCameraSceneNode::getViewFrustrum()
{
	return &ViewArea;
}


void CCameraSceneNode::recalculateViewArea()
{
	ox::core::CMatrix4 mat = Projection * View;
	ViewArea = SViewFrustrum(mat);

	ViewArea.cameraPosition = getAbsolutePosition();
	ViewArea.recalculateBoundingBox();
}



} // end namespace
} // end namespace

