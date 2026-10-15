// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleSystemSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_PARTICLE_SYSTEM_SCENE_NODE_H_INCLUDED__
#define __C_PARTICLE_SYSTEM_SCENE_NODE_H_INCLUDED__

#include "ox/scene/IParticleSystemSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IParticleAffector.h"
#include "ox/scene/IParticleEmitter.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/SParticle.h"
#include "ox/TArray.h"
#include "ox/TList.h"
#include "ox/video/S3DVertex.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! ox::core::A particle system scene node.
/** ox::core::A scene node controlling a particle System. The behavior of the particles
can be controlling by setting the right particle emitters and effectors.
*/
class CParticleSystemSceneNode : public IParticleSystemSceneNode
{
public:

	//! constructor
	CParticleSystemSceneNode(bool createDefaultEmitter,
		ISceneNode* parent, ISceneManager* mgr, int id,
		const ox::core::CVector3d<float>& position,
		const ox::core::CVector3d<float>& rotation,
		const ox::core::CVector3d<float>& scale);

	//! destructor
	virtual ~CParticleSystemSceneNode();

	//! Sets the particle emitter, which creates the particles.
	virtual void setEmitter(IParticleEmitter* emitter);

	//! Adds new particle effector to the particle system.
	virtual void addAffector(IParticleAffector* affector);

	//! Removes all particle affectors in the particle system.
	virtual void removeAllAffectors();

	//! Returns the material based on the zero based index i.
	virtual ox::video::SMaterial& getMaterial(int i);

	//! Returns amount of materials used by this scene node.
	virtual int getMaterialCount();

	//! pre render event
	virtual void OnPreRender();

	//! render
	virtual void render();

	//! returns the axis aligned bounding box of this node
	virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

	//! Creates a point particle emitter.
	virtual IParticleEmitter* createPointEmitter(
		ox::core::CVector3d<float> direction = ox::core::CVector3d<float>(0.0f,0.03f,0.0f),
		unsigned int minParticlesPerSecond = 5,
		unsigned int maxParticlePerSecond = 10,
		ox::video::SColor minStartColor = ox::video::SColor(255,0,0,0),
		ox::video::SColor maxStartColor = ox::video::SColor(255,255,255,255),
		unsigned int lifeTimeMin=2000, unsigned int lifeTimeMax=4000,
		int maxAngleDegrees=0);

	//! Creates a box particle emitter.
	virtual IParticleEmitter* createBoxEmitter(
		ox::core::CAabbox3d<float> box = ox::core::CAabbox3d<float>(-10,28,-10,10,30,10),
		ox::core::CVector3d<float> direction = ox::core::CVector3d<float>(0.0f,0.03f,0.0f),
		unsigned int minParticlesPerSecond = 5,
		unsigned int maxParticlePerSecond = 10,
		ox::video::SColor minStartColor = ox::video::SColor(255,0,0,0),
		ox::video::SColor maxStartColor = ox::video::SColor(255,255,255,255),
		unsigned int lifeTimeMin=2000, unsigned int lifeTimeMax=4000,
		int maxAngleDegrees=0);

	//! Creates a fade out particle affector.
	virtual IParticleAffector* createFadeOutParticleAffector(
		ox::video::SColor targetColor = ox::video::SColor(0,0,0,0),
		unsigned int timeNeededToFadeOut = 1000);

	//! Creates a gravity affector.
	virtual IParticleAffector* createGravityAffector(
		ox::core::CVector3d<float> gravity = ox::core::CVector3d<float>(0.0f,-0.03f,0.0f),
		unsigned int timeForceLost = 1000);

	//! Sets the size of all particles.
	virtual void setParticleSize(
		const ox::core::CDimension2d<float> &size = ox::core::CDimension2d<float>(5.0f, 5.0f));

private:

	void doParticleSystem(unsigned int time);
	void reallocateBuffers();

	ox::TList<IParticleAffector*> AffectorList;
	IParticleEmitter* Emitter;
	ox::TArray<SParticle> Particles;
	ox::video::SMaterial Material;
	ox::core::CAabbox3d<float> Box;
	unsigned int lastEmitTime;
	ox::core::CDimension2d<float> ParticleSize;

	ox::TArray<ox::video::S3DVertex> Vertices; // 4 vertices per particle
	ox::TArray<unsigned short> Indices; // 6 indices per particle
};

} // end namespace scene
} // end namespace daisy


#endif
