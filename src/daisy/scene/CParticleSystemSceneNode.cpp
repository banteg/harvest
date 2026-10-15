// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleSystemSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IParticleAffector.h"
#include "ox/scene/IParticleEmitter.h"
#include "ox/scene/IParticleSystemSceneNode.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/SParticle.h"
#include "ox/TList.h"
#include "CParticleSystemSceneNode.h"
#include "daisy/os.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/video/IVideoDriver.h"

#include "CParticlePointEmitter.h"
#include "CParticleBoxEmitter.h"
#include "CParticleFadeOutAffector.h"
#include "CParticleGravityAffector.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
CParticleSystemSceneNode::CParticleSystemSceneNode(bool createDefaultEmitter,
	ISceneNode* parent, ISceneManager* mgr, int id,
	const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& rotation,
	const ox::core::CVector3d<float>& scale)
	: IParticleSystemSceneNode(parent, mgr, id, position, rotation, scale),
	Emitter(0)
{

	if (createDefaultEmitter)
	{
		IParticleEmitter* e = createPointEmitter();
		setEmitter(e);
		e->drop();
	}

	setParticleSize();
}



//! destructor
CParticleSystemSceneNode::~CParticleSystemSceneNode()
{
	if (Emitter)
		Emitter->drop();

	removeAllAffectors();
}



//! Sets the particle emitter, which creates the particles.
void CParticleSystemSceneNode::setEmitter(IParticleEmitter* emitter)
{
	lastEmitTime = os::Timer::getTime();

	if (Emitter)
		Emitter->drop();

	Emitter = emitter;

	if (Emitter)
		Emitter->grab();
}



//! Adds new particle effector to the particle system.
void CParticleSystemSceneNode::addAffector(IParticleAffector* affector)
{
	affector->grab();
	AffectorList.push_back(affector);
}



//! Removes all particle affectors in the particle system.
void CParticleSystemSceneNode::removeAllAffectors()
{
	ox::TList<IParticleAffector*>::iterator it = AffectorList.begin();
	while (it != AffectorList.end())
	{
		(*it)->drop();
		it = AffectorList.erase(it);
	}
}


//! Returns the material based on the zero based index i.
ox::video::SMaterial& CParticleSystemSceneNode::getMaterial(int i)
{
	return Material;
}



//! Returns amount of materials used by this scene node.
int CParticleSystemSceneNode::getMaterialCount()
{
	return 1;
}



//! Creates a point particle emitter.
IParticleEmitter* CParticleSystemSceneNode::createPointEmitter(
	ox::core::CVector3d<float> direction, unsigned int minParticlesPerSecond,
	unsigned int maxParticlePerSecond, ox::video::SColor minStartColor,
	ox::video::SColor maxStartColor, unsigned int lifeTimeMin, unsigned int lifeTimeMax,
	int maxAngleDegrees)
{
	return new CParticlePointEmitter(direction, minParticlesPerSecond,
		maxParticlePerSecond, minStartColor, maxStartColor,
		lifeTimeMin, lifeTimeMax, maxAngleDegrees);
}


//! Creates a box particle emitter.
IParticleEmitter* CParticleSystemSceneNode::createBoxEmitter(
	ox::core::CAabbox3d<float> box,	ox::core::CVector3d<float> direction,
	unsigned int minParticlesPerSecond,	unsigned int maxParticlePerSecond,
	ox::video::SColor minStartColor,	ox::video::SColor maxStartColor,
	unsigned int lifeTimeMin, unsigned int lifeTimeMax,
	int maxAngleDegrees)
{
	return new CParticleBoxEmitter(box, direction, minParticlesPerSecond,
		maxParticlePerSecond, minStartColor, maxStartColor,
		lifeTimeMin, lifeTimeMax, maxAngleDegrees);
}




//! Creates a fade out particle affector.
IParticleAffector* CParticleSystemSceneNode::createFadeOutParticleAffector(
		ox::video::SColor targetColor,	unsigned int timeNeededToFadeOut)
{
	return new CParticleFadeOutAffector(targetColor, timeNeededToFadeOut);
}


//! Creates a gravity affector.
IParticleAffector* CParticleSystemSceneNode::createGravityAffector(
		ox::core::CVector3d<float> gravity, unsigned int timeForceLost)
{
	return new CParticleGravityAffector(gravity, timeForceLost);
}


//! pre render event
void CParticleSystemSceneNode::OnPreRender()
{
	if (IsVisible)
	{
		doParticleSystem(os::Timer::getTime());

		if (Particles.size() != 0)
		{
			SceneManager->registerNodeForRendering(this, ox::scene::SNRT_DEFAULT);
			ISceneNode::OnPreRender();
		}
	}
}



//! render
void CParticleSystemSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	// calculate vectors for letting particles look to camera
	ox::core::CVector3d<float> pos = getAbsolutePosition();

	ox::core::CVector3d<float> campos = camera->getAbsolutePosition();
	ox::core::CVector3d<float> target = camera->getTarget();
	ox::core::CVector3d<float> up = camera->getUpVector();
	ox::core::CVector3d<float> view = target - campos;
	view.normalize();

	ox::core::CVector3d<float> horizontal = up.crossProduct(view);
	horizontal.normalize();

	ox::core::CVector3d<float> vertical = horizontal.crossProduct(view);
	vertical.normalize();

	horizontal *= 0.5f * ParticleSize.Width;
	vertical *= 0.5f * ParticleSize.Height;

	view *= -1.0f;

	// reallocate arrays, if they are too small
	reallocateBuffers();

	// create particle vertex data
	for (unsigned int i=0; i<Particles.size(); ++i)
	{
		SParticle& particle = Particles[i];

		int idx = i*4;

		Vertices[0+idx].Pos = particle.pos + horizontal + vertical;
		Vertices[0+idx].Color = particle.color;
		Vertices[0+idx].Normal = view;

		Vertices[1+idx].Pos = particle.pos + horizontal - vertical;
		Vertices[1+idx].Color = particle.color;
		Vertices[1+idx].Normal = view;

		Vertices[2+idx].Pos = particle.pos - horizontal - vertical;
		Vertices[2+idx].Color = particle.color;
		Vertices[2+idx].Normal = view;

		Vertices[3+idx].Pos = particle.pos - horizontal + vertical;
		Vertices[3+idx].Color = particle.color;
		Vertices[3+idx].Normal = view;
	}

	// render all
	driver->setTransform(ox::video::ETS_WORLD, AbsoluteTransformation);
	driver->setMaterial(Material);

	driver->drawIndexedTriangleList(&Vertices[0], Particles.size()*4,
		&Indices[0], Particles.size()*2);

	// for debug purposes only:
	if (DebugDataVisible)
	{
		ox::video::SMaterial m;
		m.Lighting = false;
		driver->setMaterial(m);
		driver->draw3DBox(Box, ox::video::SColor(0,255,255,255));
	}
}



//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CParticleSystemSceneNode::getBoundingBox() const
{
	return Box;
}



void CParticleSystemSceneNode::doParticleSystem(unsigned int time)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! Sets the size of all particles.
void CParticleSystemSceneNode::setParticleSize(const ox::core::CDimension2d<float> &size)
{
	ParticleSize = size;
}


void CParticleSystemSceneNode::reallocateBuffers()
{
	if (Particles.size() * 4 > Vertices.size() ||
        Particles.size() * 6 > Indices.size())
	{
		int oldSize = Vertices.size();
		Vertices.resize(Particles.size() * 4);

		unsigned int i;

		// fill remaining vertices
		for (i=oldSize; i<Vertices.size(); i+=4)
		{
			Vertices[0+i].TCoords.set(0.0f, 0.0f);
			Vertices[1+i].TCoords.set(0.0f, 1.0f);
			Vertices[2+i].TCoords.set(1.0f, 1.0f);
			Vertices[3+i].TCoords.set(1.0f, 0.0f);
		}

		// fill remaining indices
		int oldIdxSize = Indices.size();
		int oldvertices = oldSize;
		Indices.resize(Particles.size() * 6);

		for (i=oldIdxSize; i<Indices.size(); i+=6)
		{
			Indices[0+i] = 0+oldvertices;
			Indices[1+i] = 2+oldvertices;
			Indices[2+i] = 1+oldvertices;
			Indices[3+i] = 0+oldvertices;
			Indices[4+i] = 3+oldvertices;
			Indices[5+i] = 2+oldvertices;
			oldvertices += 4;
		}
	}
}


} // end namespace scene
} // end namespace daisy
