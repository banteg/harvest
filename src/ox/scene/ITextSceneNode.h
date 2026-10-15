// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/ITextSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_TEXT_SCENE_NODE_H_INCLUDED__
#define __I_TEXT_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/scene/ISceneManager.h"

namespace ox
{
namespace scene
{

//! ox::core::A scene node for displaying 2d text at a position in three dimensional space
class ITextSceneNode : public ISceneNode
{
public:

	//! constructor
	ITextSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
		const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0))
			: ISceneNode(parent, mgr, id, position) {}

	//! sets the text string
	virtual void setText(const wchar_t* text) = 0;

	//! sets the color of the text
	virtual void setTextColor(ox::video::SColor color) = 0;
};

} // end namespace scene
} // end namespace ox


#endif
