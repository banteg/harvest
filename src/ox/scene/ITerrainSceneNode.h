// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ITerrainSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_TERRAIN_SCENE_NODE_H_INCLUDED__
#define __I_TERRAIN_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneManager.h"
#include "ox/core/CVector2d.h"

namespace ox
{
namespace scene
{

//! ox::core::A terrain scene node using simple geomipmapping.
//! Please note, that this is still a very early alpha version. LOD support is currently
//! disabled for example, so using this with big heightmaps will be extremely slow.
class ITerrainSceneNode : public ISceneNode
{
public:

	//! constructor
	ITerrainSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
		const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
		const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),
		const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f))
			: ISceneNode(parent, mgr, id, position, rotation, scale) {}

	//! Calculates height of a point in the terrain
	//! \param pos: 2d position of the terrain. You could take the X and ox::core::Z
	//! component of a 3d vector for this.
	//! \param outHeight: the calculated height at the 2d position will be
	//! stored in this float if the 2d position is inside the terrain.
	//! \return Returns true, if the 2d position is insied the terrain and
	//! a valid height value has been calculated. Otherwise, false is returned.
	virtual bool getHeight(ox::core::CVector2d<float> pos, float& outHeight) = 0;

	//! Sets the Level of detail to a fixed level for all tiles.
	//! Only use this for debug purposes.
	//! \param i: Level to add to the current debug level of detail,
	//! may be negative. To disable debugging LOD, set this to 0.
	virtual void setDebugLOD(int i) = 0;
};

} // end namespace scene
} // end namespace ox


#endif
